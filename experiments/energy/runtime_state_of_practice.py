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


def compile_cpp_baseline(out, clang, onnx_root):
    out.mkdir(parents=True, exist_ok=True)
    root = pathlib.Path(onnx_root).resolve()
    header = root / 'include/onnxruntime_cxx_api.h'
    libdir = root / 'lib'
    if not header.is_file() or not libdir.is_dir() or not any(libdir.glob('libonnxruntime.so*')):
        raise ValueError('verified ONNX Runtime SDK root is required for independent C++ control')
    source = HERE / 'cpp_onnx_baseline.cpp'
    binary = out / 'cpp-onnx-baseline'
    campaign.command([
        clang, '-std=c++17', '-O2', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
        source, '-I' + str(root / 'include'), '-L' + str(libdir),
        '-Wl,-rpath,' + str(libdir), '-lonnxruntime', '-pthread', '-o', binary
    ], out, 'compile-cpp-onnx')
    return binary


def validate_native_report(path, expected_predictions=None):
    report = campaign.load(path)
    if report.get('schema') != 'shorthand.ai.application.report.v1' or report.get('success') is not True:
        raise ValueError('native AIRuntime did not produce a successful application report')
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


def case_inputs(case):
    app = campaign.load(case['application'])
    q = campaign.load(app['qualification_config'])
    if q.get('model_sha256') != campaign.sha(q['model_path']):
        raise ValueError('runtime model digest mismatch')
    if app.get('dataset_sha256') != campaign.sha(app['dataset_path']):
        raise ValueError('runtime dataset digest mismatch')
    if q.get('batch_size') not in (1, 16, 32) or app.get('threads') not in (1, 2, 4):
        raise ValueError('unexpected runtime state-of-practice cell')
    return app, q


def run_case(plan, case, out, tool, cpp_binary):
    out.mkdir(parents=True, exist_ok=False)
    app, q = case_inputs(case)
    rows = 1797
    completed = rows * q['repetitions'] * q['trials']

    # Preflight is outside measured pairs and establishes exact FP32 output parity.
    native_preflight = out / 'preflight-native-report.json'
    campaign.command([tool, 'application', case['application'], native_preflight],
                     out, 'preflight-native', completed)
    expected = validate_native_report(native_preflight)['predictions']
    checksum = int(sum(expected) * q['repetitions'] * q['trials'])

    cpp_command = [
        cpp_binary, q['model_path'], app['dataset_path'], q['batch_size'], app['threads'],
        q['warmups'], q['repetitions'], q['trials']
    ]
    cpp_preflight = campaign.command(cpp_command, out, 'preflight-cpp', completed)
    check_cpp_output(out / cpp_preflight['stdout'], expected, checksum)

    orders = [['native', 'cpp_onnx'], ['cpp_onnx', 'native']] * (plan['runtime_pairs'] // 2)
    random.Random(plan['seed'] + q['batch_size'] * 31 + app['threads']).shuffle(orders)
    pairs = []
    for index, order in enumerate(orders):
        pair = dict(order=order)
        for runner in order:
            if runner == 'native':
                report_path = out / f'pair-{index}-native-report.json'
                trial = campaign.command(
                    [tool, 'application', case['application'], report_path],
                    out, f'pair-{index}-native', completed)
                validate_native_report(report_path, expected)
                trial['application_report'] = report_path.name
            else:
                trial = campaign.command(cpp_command, out, f'pair-{index}-cpp_onnx', completed)
                check_cpp_output(out / trial['stdout'], expected, checksum)
            pair[runner] = trial
        pairs.append(pair)

    report = dict(
        schema='shorthand.energy.cpp_onnx_runtime.v1',
        id=case['id'],
        boundary='whole_process_warm_os_cache',
        functional_unit='completed_classification',
        precision='float32',
        model_sha256=q['model_sha256'],
        dataset_sha256=app['dataset_sha256'],
        batch_size=q['batch_size'],
        threads=app['threads'],
        repetitions=q['repetitions'],
        trials=q['trials'],
        completed_per_process=completed,
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
        trials = [p[k] for p in report['pairs'] for k in p['order']]
        campaign.attach_energy(tool, directory, trials, trace, inputs / 'instrument.json', policy)
        campaign.write(directory / 'report.json', report)


def compare_cell(plan, report, measured, uncertainty):
    native, cpp = [], []
    for pair in report['pairs']:
        for key, values in (('native', native), ('cpp_onnx', cpp)):
            trial = pair[key]
            if trial.get('returncode') != 0 or trial.get('completed') != report['completed_per_process']:
                raise ValueError('failed or incomplete C++/ONNX runtime trial')
            value = trial['elapsed_ms'] / trial['completed']
            if measured:
                energy = trial.get('energy')
                if not energy:
                    raise ValueError('missing physical energy for C++/ONNX runtime trial')
                value = energy['joules_per_fu']
            values.append(value)
    return campaign.energy_statistics.compare(
        native, cpp, seed=plan['seed'], uncertainty_percent=uncertainty if measured else 0)


def build_summary(plan, reports, measured, uncertainty):
    cells = []
    for cell, report, _ in sorted(reports):
        cells.append(dict(
            id=cell,
            metric='joules_per_image' if measured else 'ms_per_image',
            comparison=compare_cell(plan, report, measured, uncertainty)))
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

        cpp_binary = compile_cpp_baseline(out / 'build', clang, args.onnxruntime_root)
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

        summary = build_summary(plan, reports, measured, uncertainty)
        campaign.write(out / 'summary.json', summary)
        write_markdown(out, summary)
        campaign.write(out / 'environment.json', dict(
            platform=platform.platform(),
            available_cpus=available,
            affinity=sorted(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else None,
            onnxruntime_root=str(pathlib.Path(args.onnxruntime_root).resolve()),
            clang_version=subprocess.check_output([clang, '--version'], text=True),
            tool_sha256=campaign.sha(tool),
            cpp_baseline_sha256=campaign.sha(cpp_binary),
            cpp_source_sha256=campaign.sha(HERE / 'cpp_onnx_baseline.cpp'),
            completed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat()))

        campaign.verify_files(plan_path.parent, plan['hashes'])
        campaign.verify_files(ROOT, plan['data_hashes'])
        if campaign.sha(plan_path) != args.plan_sha256:
            raise ValueError('plan changed during independent C++/ONNX capture')

        hashes = {str(p.relative_to(out)): campaign.sha(p)
                  for p in sorted(out.rglob('*')) if p.is_file() and p.name != 'manifest.json'}
        manifest = dict(
            schema='shorthand.energy.cpp_onnx_runtime.bundle.v1',
            success=True,
            plan_sha256=args.plan_sha256,
            hashes=hashes,
            **CLAIMS)
        campaign.write(out / 'manifest.json', manifest)
        replay = analyze(out, campaign.sha(out / 'manifest.json'))
        if replay != summary:
            raise ValueError('independent C++/ONNX replay differs from captured summary')
        print(f'bundle={out}\nmanifest_sha256={campaign.sha(out / "manifest.json")}')
    except Exception as error:
        campaign.write(out / 'failure.json', dict(success=False, error=str(error), **CLAIMS))
        raise


def analyze(out, expected_sha):
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
    reports = []
    for case in plan['runtime_cases']:
        directory = out / 'cells' / case['id']
        report = campaign.load(directory / 'report.json')
        if report.get('id') != case['id'] or len(report.get('pairs', [])) != plan['runtime_pairs']:
            raise ValueError('incomplete C++/ONNX runtime report: ' + case['id'])
        orders = [p['order'] for p in report['pairs']]
        if orders.count(['native', 'cpp_onnx']) != plan['runtime_pairs'] // 2 or orders.count(['cpp_onnx', 'native']) != plan['runtime_pairs'] // 2:
            raise ValueError('unbalanced C++/ONNX runner order: ' + case['id'])
        reports.append((case['id'], report, directory))

    summary = build_summary(plan, reports, measured, uncertainty)
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
        q['repetitions'] = 1
        q['trials'] = 1
        q['warmups'] = 1
        campaign.write(qpath, q)
        app['qualification_sha256'] = campaign.sha(qpath)
        campaign.write(app_path, app)

        cpp = compile_cpp_baseline(root / 'build', resolve_executable(clang), onnx_root)
        native_path = root / 'native.json'
        campaign.command([tool, 'application', app_path, native_path], root, 'native', 1797)
        expected = validate_native_report(native_path)['predictions']
        trial = campaign.command(
            [cpp, q['model_path'], app['dataset_path'], 16, 1, 1, 1, 1],
            root, 'cpp', 1797)
        check_cpp_output(root / trial['stdout'], expected, int(sum(expected)))
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

    args = parser.parse_args()
    if args.action == 'run':
        run(args)
    else:
        analyze(args.bundle, args.manifest_sha256)


if __name__ == '__main__':
    main()
