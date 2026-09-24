#!/usr/bin/env python3
"""Independent C++/ONNX Runtime control for PR104 FP32 runtime experiments.

The existing campaign compares Shorthand AIRuntime with Python+ONNX Runtime.
This experiment adds the critical compiled control: the same ONNX model, data,
batch/thread settings and ORT session controls executed by a standalone C++17
program that does not link or call Shorthand runtime code.
"""
import argparse
import datetime
import math
import os
import pathlib
import platform
import random
import shutil
import statistics
import subprocess
import tempfile
import time

import campaign

ROOT = pathlib.Path(__file__).resolve().parents[2]
HERE = pathlib.Path(__file__).resolve().parent
CLAIMS = campaign.CLAIMS


def resolve_executable(value):
    path = pathlib.Path(shutil.which(str(value)) or value).absolute()
    if not path.is_file():
        raise ValueError('missing executable: ' + str(value))
    return path


def compile_cpp_baseline(out, clang, onnx_root, source=None):
    out.mkdir(parents=True, exist_ok=True)
    root = pathlib.Path(onnx_root).resolve()
    header = root / 'include/onnxruntime_cxx_api.h'
    libdir = root / 'lib'
    if not header.is_file() or not libdir.is_dir() or not any(libdir.glob('libonnxruntime.so*')):
        raise ValueError('verified ONNX Runtime SDK root is required for independent C++ control')
    source = pathlib.Path(source) if source else HERE / 'cpp_onnx_baseline.cpp'
    if not source.is_file():
        raise ValueError('missing frozen independent C++ baseline source')
    binary = out / 'cpp-onnx-baseline'
    campaign.command([
        clang, '-std=c++17', '-O3', '-fno-fast-math', '-DNDEBUG',
        '-Wall', '-Wextra', '-Wpedantic', '-Werror',
        source, '-I' + str(root / 'include'), '-L' + str(libdir),
        '-Wl,-rpath,' + str(libdir), '-lonnxruntime', '-pthread', '-o', binary
    ], out, 'compile-cpp-onnx')
    env = dict(os.environ)
    env['LD_LIBRARY_PATH'] = str(libdir) + ':' + env.get('LD_LIBRARY_PATH', '')
    version = subprocess.check_output([binary, '--version'], text=True, env=env).strip()
    if version != '1.30.0':
        raise ValueError('independent C++ control requires ONNX Runtime 1.30.0, observed ' + version)
    return binary


def validate_native_report(path, expected_predictions=None):
    report = campaign.load(path)
    if report.get('schema') != 'shorthand.ai.application.report.v1' or report.get('success') is not True:
        reasons = [trial.get('reason') for trial in report.get('trials', [])
                   if isinstance(trial, dict) and trial.get('success') is not True]
        raise ValueError(
            'native AIRuntime did not produce a successful application report: '
            f'schema={report.get("schema")!r}, success={report.get("success")!r}, '
            f'error={report.get("error")!r}, reason={report.get("reason")!r}, '
            f'trial_reasons={reasons!r}, keys={sorted(report)!r}')
    predictions = report.get('predictions')
    if not isinstance(predictions, list) or len(predictions) != 1797:
        raise ValueError('native AIRuntime prediction shape mismatch')
    if expected_predictions is not None and predictions != expected_predictions:
        raise ValueError('native AIRuntime predictions changed within paired experiment')
    accuracy = report.get('accuracy')
    if not isinstance(accuracy, (int, float)) or not math.isfinite(accuracy) or accuracy < .85:
        raise ValueError('native AIRuntime quality below predeclared threshold')
    return report


def check_cpp_output(path, predictions, checksum):
    values = pathlib.Path(path).read_text().split()
    if len(values) != len(predictions) + 1:
        raise ValueError('independent C++ output length mismatch')
    try:
        observed_predictions = [int(v) for v in values[:-1]]
        observed_checksum = int(values[-1])
    except ValueError as exc:
        raise ValueError('independent C++ emitted non-integer output') from exc
    if observed_predictions != predictions or observed_checksum != checksum:
        raise ValueError('independent C++ prediction/checksum mismatch')


def load_cpp_trials(path, q):
    rows = []
    with pathlib.Path(path).open() as stream:
        header = stream.readline().strip()
        if header != 'start_unix_seconds,end_unix_seconds,elapsed_ms,completed':
            raise ValueError('invalid C++ trial report header')
        for line in stream:
            fields = line.strip().split(',')
            if len(fields) != 4:
                raise ValueError('invalid C++ trial report row')
            start, end, elapsed = map(float, fields[:3])
            completed = int(fields[3])
            if not all(map(math.isfinite, (start, end, elapsed))) or end <= start or elapsed <= 0:
                raise ValueError('invalid C++ trial timing')
            if completed != 1797 * q['repetitions']:
                raise ValueError('incomplete C++ inner trial')
            if abs((end - start) * 1000 - elapsed) > max(10.0, elapsed * .05):
                raise ValueError('C++ wall/steady clocks disagree')
            rows.append(dict(start_unix_seconds=start, end_unix_seconds=end,
                             elapsed_ms=elapsed, completed=completed, success=True))
    if len(rows) != q['trials']:
        raise ValueError('C++ inner trial count mismatch')
    return rows


def check_cpp_validation(path, native):
    """Compare logits and stable top-3 across implementations outside timing."""
    report = campaign.load(str(path) + '.validation.json')
    scores = report.get('scores', [])
    reference = native.get('scores', [])
    if len(scores) != len(native['predictions']) * 10 or len(scores) != len(reference):
        raise ValueError('independent C++ score shape mismatch')
    if any(not math.isfinite(a) or not math.isfinite(b) or abs(a - b) > 1e-5 + 1e-4 * abs(b)
           for a, b in zip(scores, reference)):
        raise ValueError('independent C++ score mismatch')
    ranked = []
    for offset in range(0, len(reference), 10):
        ranked.extend(sorted(range(10), key=lambda label: (-reference[offset + label], label))[:3])
    if report.get('top_k') != ranked:
        raise ValueError('independent C++ top-3 mismatch')


def case_inputs(case):
    app = campaign.load(case['application'])
    q = campaign.load(app['qualification_config'])
    if q.get('model_sha256') != campaign.sha(q['model_path']):
        raise ValueError('runtime model digest mismatch')
    if app.get('dataset_sha256') != campaign.sha(app['dataset_path']):
        raise ValueError('runtime dataset digest mismatch')
    if q.get('batch_size') not in (1, 16, 32) or app.get('threads') not in (1, 2, 4):
        raise ValueError('unexpected runtime state-of-practice cell')
    expected_application = dict(features=64, classes=10, top_k=3, input_min=0,
                                input_max=16, offset=0, scale=0.0625,
                                minimum_accuracy=0.85)
    if any(app.get(key) != value for key, value in expected_application.items()):
        raise ValueError('independent C++ control no longer matches application semantics: ' + case['id'])
    if q.get('workload') != 'uci_optdigits_centroid' or q.get('input_shape') != [q['batch_size'], 64] or q.get('output_shape') != [q['batch_size'], 10]:
        raise ValueError('independent C++ control no longer matches ONNX workload shape')
    return app, q


def run_case(plan, case, out, tool, cpp_binary):
    out.mkdir(parents=True, exist_ok=False)
    app, q = case_inputs(case)
    rows = 1797
    completed_per_trial = rows * q['repetitions']
    completed_per_process = completed_per_trial * q['trials']

    # Preflight is outside measured pairs and establishes exact FP32 output parity.
    native_preflight = out / 'preflight-native-report.json'
    campaign.command([tool, 'application', case['application'], native_preflight],
                     out, 'preflight-native', completed_per_process)
    reference = validate_native_report(native_preflight)
    expected = reference['predictions']
    checksum = int(sum(expected) * q['repetitions'] * q['trials'])

    cpp_base = [
        cpp_binary, q['model_path'], app['dataset_path'], q['batch_size'], app['threads'],
        q['warmups'], q['repetitions'], q['trials']
    ]
    preflight_trials = out / 'preflight-cpp-trials.csv'
    cpp_preflight = campaign.command(cpp_base + [preflight_trials], out, 'preflight-cpp', completed_per_process)
    check_cpp_output(out / cpp_preflight['stdout'], expected, checksum)
    check_cpp_validation(preflight_trials, reference)
    load_cpp_trials(preflight_trials, q)

    orders = [['native', 'cpp_onnx'], ['cpp_onnx', 'native']] * (plan['runtime_pairs'] // 2)
    random.Random(plan['seed'] + q['batch_size'] * 31 + app['threads']).shuffle(orders)
    pairs = []
    for index, order in enumerate(orders):
        pair = dict(order=order)
        for runner in order:
            if runner == 'native':
                report_path = out / f'pair-{index}-native-report.json'
                process = campaign.command(
                    [tool, 'application', case['application'], report_path],
                    out, f'pair-{index}-native', completed_per_process)
                native = validate_native_report(report_path, expected)
                if len(native.get('trials', [])) != q['trials']:
                    raise ValueError('native inner trial count mismatch')
                if any(t.get('completed') != completed_per_trial for t in native['trials']):
                    raise ValueError('native inner trial functional unit mismatch')
                process['application_report'] = report_path.name
                pair['native'] = process
                pair['native_trials'] = native['trials']
            else:
                trial_path = out / f'pair-{index}-cpp-trials.csv'
                process = campaign.command(
                    cpp_base + [trial_path], out, f'pair-{index}-cpp_onnx', completed_per_process)
                check_cpp_output(out / process['stdout'], expected, checksum)
                check_cpp_validation(trial_path, reference)
                process['trial_report'] = trial_path.name
                pair['cpp_onnx'] = process
                pair['cpp_trials'] = load_cpp_trials(trial_path, q)
        pairs.append(pair)

    report = dict(
        schema='shorthand.energy.cpp_onnx_runtime.v1',
        id=case['id'],
        boundary='resident_session_inner_trial',
        functional_unit='completed_classification',
        precision='float32',
        model_sha256=q['model_sha256'],
        dataset_sha256=app['dataset_sha256'],
        batch_size=q['batch_size'],
        threads=app['threads'],
        repetitions=q['repetitions'],
        trials=q['trials'],
        completed_per_trial=completed_per_trial,
        pairs=pairs,
        **CLAIMS)
    campaign.write(out / 'report.json', report)
    return report


def attach_energy(plan, inputs, out, tool, reports):
    time.sleep(1.0)
    trace = out / 'meter.csv'
    campaign.snapshot(pathlib.Path(plan['meter_csv']), trace)
    if not trace.read_bytes().endswith(b'\n'):
        raise ValueError('incomplete live meter trace snapshot')
    policy = campaign.load(inputs / 'policy.json')
    for _, report, directory in reports:
        trials = [trial for pair in report['pairs']
                  for key in ('native_trials', 'cpp_trials') for trial in pair[key]]
        campaign.attach_energy(tool, directory, trials, trace, inputs / 'instrument.json', policy)
        campaign.write(directory / 'report.json', report)


def compare_cell(plan, report, measured, uncertainty, policy):
    native, cpp = [], []
    for pair in report['pairs']:
        for key, values in (('native_trials', native), ('cpp_trials', cpp)):
            inner = pair[key]
            if len(inner) != report['trials']:
                raise ValueError('incomplete C++/ONNX inner trial set')
            observations = []
            for trial in inner:
                if trial.get('completed') != report['completed_per_trial']:
                    raise ValueError('C++/ONNX inner trial functional unit mismatch')
                value = trial['elapsed_ms'] / trial['completed']
                if measured:
                    energy = trial.get('energy')
                    if not energy:
                        raise ValueError('missing physical energy for C++/ONNX inner trial')
                    value = energy['joules_per_fu']
                observations.append(value)
            values.append(statistics.mean(observations))
    for values in (native, cpp):
        variability = 100 * statistics.stdev(values) / statistics.mean(values)
        limit = policy['maximum_trial_variability_percent']
        if variability > limit:
            raise ValueError('C++/ONNX process-pair variability exceeds declared policy')
        if measured and uncertainty + 2 * variability > policy['maximum_uncertainty_percent']:
            raise ValueError('C++/ONNX energy uncertainty exceeds declared policy')
    return campaign.energy_statistics.compare(
        native, cpp, seed=plan['seed'], uncertainty_percent=uncertainty if measured else 0)


def build_summary(plan, reports, measured, uncertainty, policy):
    cells = []
    for cell, report, _ in sorted(reports):
        cells.append(dict(
            id=cell,
            metric='joules_per_image' if measured else 'ms_per_image',
            comparison=compare_cell(plan, report, measured, uncertainty, policy)))
    return dict(
        schema='shorthand.energy.cpp_onnx_runtime.summary.v1',
        scope='same FP32 ONNX workload: Shorthand AIRuntime versus independent C++17 ONNX Runtime',
        mode=plan['mode'],
        cells=cells,
        energy_evidence_qualified=measured,
        comparative_energy_claim=False,
        lowest_carbon_language_claim=False,
        official_certification_granted=False)


def write_markdown(out, summary):
    lines = [
        '# Independent C++ / ONNX Runtime control', '',
        f'Mode: {summary["mode"]}.',
        'Each row compares Shorthand AIRuntime against standalone C++17 using the same ONNX Runtime CPU provider and session controls.', '',
        '| Cell | Metric | Shorthand/native mean | C++/ONNX mean | Shorthand reduction % | Paired 95% interval % |',
        '| --- | --- | ---: | ---: | ---: | --- |'
    ]
    for cell in summary['cells']:
        c = cell['comparison']
        lines.append(
            f'| {cell["id"]} | {cell["metric"]} | {c["native_mean"]:.6g} | {c["python_mean"]:.6g} | '
            f'{c["savings_percent"]:.3f} | {c["paired_bootstrap_95_percent_interval"]} |')
    lines += [
        '',
        'Positive reduction means the Shorthand/native side used less of the measured quantity.',
        'Negative reduction means the independent C++/ONNX control used less.',
        'This control isolates runtime/integration overhead; it is not merged with the FP64 source-language matrix.'
    ]
    (out / 'summary.md').write_text('\n'.join(lines) + '\n')


def run(args):
    plan_path = args.plan.resolve()
    if campaign.sha(plan_path) != args.plan_sha256:
        raise ValueError('predeclared campaign plan digest mismatch')
    plan = campaign.load(plan_path)
    campaign.validate_plan(plan)
    campaign.verify_files(plan_path.parent, plan['hashes'])
    campaign.verify_files(ROOT, plan['data_hashes'])
    measured = plan['mode'] == 'calibrated_energy'

    if platform.system() != 'Linux' or platform.machine() != 'x86_64':
        raise ValueError('independent C++/ONNX control is scoped to Linux x64')
    available = len(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else os.cpu_count()
    if not available or available < 4:
        raise ValueError('C++/ONNX control requires four available CPU threads')

    clang = resolve_executable(args.clang)
    tool = resolve_executable(args.tool)
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    try:
        inputs = out / 'inputs'
        inputs.mkdir()
        for name in plan['hashes']:
            target = inputs / name
            target.parent.mkdir(parents=True, exist_ok=True)
            campaign.snapshot(plan_path.parent / name, target)
        campaign.snapshot(plan_path, out / 'plan.json')
        campaign.verify_files(inputs, plan['hashes'])

        implementation = out / 'implementation'
        implementation.mkdir()
        campaign.snapshot(HERE / 'cpp_onnx_baseline.cpp', implementation / 'cpp_onnx_baseline.cpp')
        campaign.snapshot(HERE / 'runtime_state_of_practice.py', implementation / 'runtime_state_of_practice.py')
        campaign.snapshot(HERE / 'campaign.py', implementation / 'campaign.py')
        campaign.snapshot(HERE / 'analysis.py', implementation / 'analysis.py')
        implementation_sha256 = {
            p.name: campaign.sha(p) for p in sorted(implementation.iterdir()) if p.is_file()
        }
        clang_sha256 = campaign.sha(clang)
        tool_sha256 = campaign.sha(tool)
        ort_root = pathlib.Path(args.onnxruntime_root).resolve()
        ort_library = ort_root / 'lib/libonnxruntime.so'
        if not ort_library.is_file():
            raise ValueError('ONNX Runtime shared library is required for evidence identity')
        ort_library_sha256 = campaign.sha(ort_library)

        cpp_binary = compile_cpp_baseline(out / 'build', clang, args.onnxruntime_root,
                                          implementation / 'cpp_onnx_baseline.cpp')
        cases = list(plan['runtime_cases'])
        random.Random(plan['seed']).shuffle(cases)
        reports = []
        for case in cases:
            directory = out / 'cells' / case['id']
            report = run_case(plan, case, directory, tool, cpp_binary)
            reports.append((case['id'], report, directory))

        uncertainty = 0
        if measured:
            attach_energy(plan, inputs, out, str(tool), reports)
            uncertainty = campaign.load(inputs / 'instrument.json')['uncertainty_percent']

        summary = build_summary(plan, reports, measured, uncertainty, campaign.load(inputs / 'policy.json'))
        campaign.write(out / 'summary.json', summary)
        write_markdown(out, summary)
        campaign.write(out / 'environment.json', dict(
            platform=platform.platform(),
            available_cpus=available,
            affinity=sorted(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else None,
            onnxruntime_root=str(pathlib.Path(args.onnxruntime_root).resolve()),
            onnxruntime_version='1.30.0',
            onnxruntime_library_sha256=ort_library_sha256,
            clang_version=subprocess.check_output([clang, '--version'], text=True),
            clang_sha256=clang_sha256,
            tool_sha256=tool_sha256,
            cpp_baseline_sha256=campaign.sha(cpp_binary),
            implementation_sha256=implementation_sha256,
            completed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat()))

        campaign.verify_files(plan_path.parent, plan['hashes'])
        campaign.verify_files(ROOT, plan['data_hashes'])
        if campaign.sha(plan_path) != args.plan_sha256:
            raise ValueError('plan changed during independent C++/ONNX capture')
        if campaign.sha(clang) != clang_sha256 or campaign.sha(tool) != tool_sha256:
            raise ValueError('compiler or Shorthand runtime tool changed during capture')
        if campaign.sha(ort_library) != ort_library_sha256:
            raise ValueError('ONNX Runtime shared library changed during capture')
        if any(campaign.sha(implementation / name) != digest
               for name, digest in implementation_sha256.items()):
            raise ValueError('retained experiment implementation changed during capture')

        hashes = {str(p.relative_to(out)): campaign.sha(p)
                  for p in sorted(out.rglob('*')) if p.is_file() and p.name != 'manifest.json'}
        manifest = dict(
            schema='shorthand.energy.cpp_onnx_runtime.bundle.v1',
            success=True,
            plan_sha256=args.plan_sha256,
            hashes=hashes,
            **CLAIMS)
        campaign.write(out / 'manifest.json', manifest)
        replay = analyze(out, campaign.sha(out / 'manifest.json'), str(tool))
        if replay != summary:
            raise ValueError('independent C++/ONNX replay differs from captured summary')
        print(f'bundle={out}\nmanifest_sha256={campaign.sha(out / "manifest.json")}')
    except Exception as error:
        campaign.write(out / 'failure.json', dict(success=False, error=str(error), **CLAIMS))
        raise


def analyze(out, expected_sha, tool=None):
    out = pathlib.Path(out).resolve()
    if campaign.sha(out / 'manifest.json') != expected_sha:
        raise ValueError('trusted C++/ONNX bundle digest mismatch')
    manifest = campaign.load(out / 'manifest.json')
    if manifest.get('schema') != 'shorthand.energy.cpp_onnx_runtime.bundle.v1' or manifest.get('success') is not True:
        raise ValueError('complete C++/ONNX bundle required')
    campaign.verify_files(out, manifest['hashes'])
    plan = campaign.load(out / 'plan.json')
    campaign.validate_plan(plan)
    if campaign.sha(out / 'plan.json') != manifest['plan_sha256']:
        raise ValueError('C++/ONNX plan integrity failure')
    measured = plan['mode'] == 'calibrated_energy'
    uncertainty = campaign.load(out / 'inputs/instrument.json')['uncertainty_percent'] if measured else 0
    environment = campaign.load(out / 'environment.json')
    if measured and not tool:
        raise ValueError('physical replay requires the native meter-window tool')
    if measured and campaign.sha(resolve_executable(tool)) != environment.get('tool_sha256'):
        raise ValueError('physical replay tool differs from the captured meter-window implementation')
    policy = campaign.load(out / 'inputs/policy.json')
    reports = []
    with tempfile.TemporaryDirectory(prefix='cpp-onnx-replay-') as temp:
        scratch = pathlib.Path(temp)
        for case in plan['runtime_cases']:
            directory = out / 'cells' / case['id']
            report = campaign.load(directory / 'report.json')
            if report.get('id') != case['id'] or len(report.get('pairs', [])) != plan['runtime_pairs']:
                raise ValueError('incomplete C++/ONNX runtime report: ' + case['id'])
            orders = [p['order'] for p in report['pairs']]
            if orders.count(['native', 'cpp_onnx']) != plan['runtime_pairs'] // 2 or orders.count(['cpp_onnx', 'native']) != plan['runtime_pairs'] // 2:
                raise ValueError('unbalanced C++/ONNX runner order: ' + case['id'])
            reference = validate_native_report(directory / 'preflight-native-report.json')
            expected = reference['predictions']
            check_cpp_validation(directory / 'preflight-cpp-trials.csv', reference)
            checksum = int(sum(expected) * report['repetitions'] * report['trials'])
            for pair in report['pairs']:
                native_process = pair['native']
                cpp_process = pair['cpp_onnx']
                native = validate_native_report(directory / native_process['application_report'], expected)
                check_cpp_output(directory / cpp_process['stdout'], expected, checksum)
                check_cpp_validation(directory / cpp_process['trial_report'], native)
                if len(native['trials']) != len(pair['native_trials']):
                    raise ValueError('retained native inner trial count changed')
                for original, retained in zip(native['trials'], pair['native_trials']):
                    if any(retained.get(k) != v for k, v in original.items()):
                        raise ValueError('retained native inner trial changed')
                q = campaign.load(out / 'inputs' / case['id'] / 'qualification.json')
                parsed_cpp = load_cpp_trials(directory / cpp_process['trial_report'], q)
                if len(parsed_cpp) != len(pair['cpp_trials']):
                    raise ValueError('retained C++ inner trial count changed')
                for original, retained in zip(parsed_cpp, pair['cpp_trials']):
                    if any(retained.get(k) != v for k, v in original.items()):
                        raise ValueError('retained C++ inner trial changed')
                if measured:
                    for trial in pair['native_trials'] + pair['cpp_trials']:
                        recorded = trial.get('energy')
                        if not recorded:
                            raise ValueError('missing physical C++/ONNX energy during replay')
                        replay_trial = dict(trial)
                        replay_trial.pop('energy', None)
                        campaign.attach_energy(tool, scratch, [replay_trial], out / 'meter.csv',
                                               out / 'inputs/instrument.json', policy)
                        if replay_trial['energy'] != recorded:
                            raise ValueError('C++/ONNX physical-meter replay mismatch')
            reports.append((case['id'], report, directory))

    summary = build_summary(plan, reports, measured, uncertainty, policy)
    if campaign.load(out / 'summary.json') != summary:
        raise ValueError('stored C++/ONNX summary is not replayable')
    return summary


def smoke(clang, tool, onnx_root):
    """CI smoke for the independent C++/ONNX executable only."""
    with tempfile.TemporaryDirectory(prefix='shorthand-cpp-onnx-smoke-') as temp:
        root = pathlib.Path(temp)
        app_path = campaign.create(root / 'fixture', batch=16, threads=1)
        app = campaign.load(app_path)
        qpath = pathlib.Path(app['qualification_config'])
        q = campaign.load(qpath)
        total_completed = 1797 * q['repetitions'] * q['trials']
        per_trial_completed = 1797 * q['repetitions']

        cpp = compile_cpp_baseline(root / 'build', resolve_executable(clang), onnx_root)
        native_path = root / 'native-report.json'
        campaign.command([tool, 'application', app_path, native_path], root, 'native', total_completed)
        expected = validate_native_report(native_path)['predictions']
        trial_report = root / 'cpp-trials.csv'
        trial = campaign.command(
            [cpp, q['model_path'], app['dataset_path'], q['batch_size'], app['threads'],
             q['warmups'], q['repetitions'], q['trials'], trial_report],
            root, 'cpp', total_completed)
        check_cpp_output(
            root / trial['stdout'], expected,
            int(sum(expected) * q['repetitions'] * q['trials']))
        check_cpp_validation(trial_report, campaign.load(native_path))
        observed = load_cpp_trials(trial_report, q)
        if len(observed) != q['trials'] or any(
                item['completed'] != per_trial_completed for item in observed):
            raise AssertionError('invalid independent C++ inner trial report')
    print('PASS independent C++/ONNX baseline matches native FP32 predictions')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='action', required=True)

    capture = commands.add_parser('run')
    capture.add_argument('--plan', type=pathlib.Path, required=True,
                         help='plan.json produced by campaign.py prepare')
    capture.add_argument('--plan-sha256', required=True)
    capture.add_argument('--clang', default='clang++-18')
    capture.add_argument('--tool', required=True)
    capture.add_argument('--onnxruntime-root', required=True)
    capture.add_argument('--output', type=pathlib.Path, required=True)

    replay = commands.add_parser('analyze')
    replay.add_argument('--bundle', type=pathlib.Path, required=True)
    replay.add_argument('--manifest-sha256', required=True)
    replay.add_argument('--tool', help='required for calibrated-energy replay')

    args = parser.parse_args()
    if args.action == 'run':
        run(args)
    else:
        analyze(args.bundle, args.manifest_sha256, args.tool)


if __name__ == '__main__':
    main()
