#!/usr/bin/env python3
"""Balanced state-of-practice source-language benchmark for PR104 energy evidence.

This is deliberately separate from campaign.py's FP32 ONNX runtime track.  It
compares the same FP64 Optdigits workload against stronger source/runtime
approaches without mixing precision/model boundaries.
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
import sys
import tempfile
import time

import campaign
from source_workload import prepare as prepare_source

ROOT = pathlib.Path(__file__).resolve().parents[2]
HERE = pathlib.Path(__file__).resolve().parent
CLAIMS = dict(comparative_energy_claim=False, lowest_carbon_language_claim=False,
              official_certification_granted=False)
CORE_BASELINES = ('numpy', 'cpp17-o3')
FULL_BASELINES = ('numpy', 'cpp17-o3', 'pytorch-eager', 'pytorch-compile', 'rust-candle', 'mojo-max')


def _bounded(value, low, high, name):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f'{name} must be an integer in [{low}, {high}]')


def _tool_version(python):
    probe = (
        'import hashlib, importlib.metadata as md, json, numpy, sys, torch; '
        'rec=lambda n: (md.distribution(n).read_text("RECORD") or "").encode(); '
        'cfg=torch.__config__.show().encode(); '
        'print(json.dumps({'
        '"python":sys.version.split()[0],'
        '"numpy":numpy.__version__,'
        '"numpy_record_sha256":hashlib.sha256(rec("numpy")).hexdigest(),'
        '"torch":torch.__version__,'
        '"torch_git_version":getattr(torch.version,"git_version",None),'
        '"torch_cuda_version":getattr(torch.version,"cuda",None),'
        '"torch_config_sha256":hashlib.sha256(cfg).hexdigest(),'
        '"torch_record_sha256":hashlib.sha256(rec("torch")).hexdigest()'
        '},sort_keys=True))'
    )
    result = subprocess.run([str(python), '-c', probe],
        text=True, capture_output=True, timeout=60)
    if result.returncode:
        raise ValueError('full profile requires an importable PyTorch environment: ' + result.stderr[-1000:])
    import json
    value = json.loads(result.stdout)
    if value['python'] != '3.12' and not value['python'].startswith('3.12.'):
        raise ValueError('full profile requires CPython 3.12 for the pinned workload')
    if value['numpy'] != '2.3.5':
        raise ValueError('full profile requires pinned NumPy 2.3.5 in the PyTorch environment')
    return value


def _external(path, version, name):
    if not path or not version:
        raise ValueError(f'full profile requires --{name}-command and --{name}-version')
    resolved = pathlib.Path(path).expanduser().resolve()
    if resolved.is_symlink() or not resolved.is_file():
        raise ValueError(f'invalid {name} runner')
    probe = subprocess.run([str(resolved), '--version'], text=True, capture_output=True, timeout=60)
    observed = probe.stdout.strip()
    if probe.returncode or not observed or observed != version:
        raise ValueError(f'{name} runner --version must exactly match the frozen version declaration')
    return dict(path=str(resolved), sha256=campaign.sha(resolved), version=observed,
                contract='support --version; accept --model MODEL.json; read canonical workload on stdin; emit predictions and checksum on stdout')


def validate_plan(plan):
    if plan.get('schema') != 'shorthand.energy.state-of-practice.plan.v1':
        raise ValueError('invalid state-of-practice plan')
    if plan.get('mode') not in ('execution_only', 'calibrated_energy') or plan.get('profile') not in ('core', 'full'):
        raise ValueError('invalid state-of-practice mode/profile')
    _bounded(plan.get('pairs'), 4, 100, 'pairs')
    _bounded(plan.get('repetitions'), 1, 10000, 'repetitions')
    if plan['pairs'] % 2 or plan.get('warmups') != 2:
        raise ValueError('balanced even pairs and exactly two warmups are required')
    expected = CORE_BASELINES if plan['profile'] == 'core' else FULL_BASELINES
    if tuple(plan.get('baselines', ())) != expected:
        raise ValueError('baseline matrix is incomplete or reordered')
    if plan['profile'] == 'full':
        if set(plan.get('external', {})) != {'rust-candle', 'mojo-max'} or not plan.get('torch'):
            raise ValueError('full profile requires PyTorch, Rust/Candle and Mojo/MAX declarations')
    if plan['mode'] == 'calibrated_energy' and (plan['pairs'] < 30 or not plan.get('meter_csv')):
        raise ValueError('physical matrix requires >=30 pairs and live whole-host telemetry')
    if plan['mode'] == 'execution_only' and plan.get('meter_csv'):
        raise ValueError('execution-only plan cannot include energy telemetry')
    if any(plan.get(key) is not value for key, value in CLAIMS.items()):
        raise ValueError('experiment plan cannot authorize claims')


def prepare(args):
    out = args.output.resolve()
    _bounded(args.repetitions, 1, 10000, 'repetitions')
    _bounded(args.pairs, 4, 100, 'pairs')
    if args.pairs % 2:
        raise ValueError('pairs must be even')
    measured = args.mode == 'calibrated_energy'
    if measured and (args.pairs < 30 or not args.instrument or not args.meter_csv):
        raise ValueError('calibrated profile requires >=30 pairs, instrument and meter CSV')
    if not measured and (args.instrument or args.meter_csv):
        raise ValueError('execution-only profile cannot accept energy evidence')
    out.mkdir(parents=True, exist_ok=False)
    prepare_source(out / 'source', args.repetitions)
    policy_name = 'comparison_measurement_policy.json' if measured else 'comparison_execution_policy.json'
    policy = campaign.load(ROOT / 'tests/ai_application' / policy_name)
    campaign.write(out / 'policy.json', policy)
    if measured:
        campaign.snapshot(args.instrument, out / 'instrument.json')
    plan = dict(schema='shorthand.energy.state-of-practice.plan.v1', mode=args.mode,
                profile=args.profile, pairs=args.pairs, repetitions=args.repetitions,
                warmups=2, seed=104, baselines=list(CORE_BASELINES if args.profile == 'core' else FULL_BASELINES),
                meter_csv=str(args.meter_csv.resolve()) if measured else None,
                scope='UCI Optdigits FP64 nearest-centroid source workload on Linux x64 CPU',
                external={}, torch=None, **CLAIMS)
    if args.profile == 'full':
        torch_python = pathlib.Path(os.path.abspath(os.path.expanduser(args.pytorch_python or sys.executable)))
        if not torch_python.is_file():
            raise ValueError('invalid PyTorch Python executable')
        plan['torch'] = dict(python=str(torch_python), sha256=campaign.sha(torch_python), versions=_tool_version(torch_python),
                             compile_cache='warm-after-two-predeclared-warmups')
        plan['external'] = {
            'rust-candle': _external(args.rust_command, args.rust_version, 'rust'),
            'mojo-max': _external(args.mojo_command, args.mojo_version, 'mojo'),
        }
    plan['hashes'] = {str(p.relative_to(out)): campaign.sha(p) for p in sorted(out.rglob('*')) if p.is_file()}
    validate_plan(plan)
    campaign.write(out / 'plan.json', plan)
    print(f'plan={out / "plan.json"}\nplan_sha256={campaign.sha(out / "plan.json")}')


def _write_cpp_source(model_path, target):
    """Generate a static-model C++ control so startup does not pay JSON parsing."""
    model = campaign.load(model_path)
    weights = model.get('weights', [])
    bias = model.get('bias', [])
    if len(weights) != 640 or len(bias) != 10 or model.get('precision') != 'float64':
        raise ValueError('invalid FP64 model for C++ control')
    w = ','.join(format(float(v), '.17g') for v in weights)
    b = ','.join(format(float(v), '.17g') for v in bias)
    target.write_text(f'''#include <array>\n#include <cmath>\n#include <iostream>\n#include <stdexcept>\n#include <vector>\n\nstatic constexpr std::array<double, 640> weights = {{{w}}};\nstatic constexpr std::array<double, 10> bias = {{{b}}};\n\nint main() {{\n  try {{\n    long long repeats_ll = 0;\n    if (!(std::cin >> repeats_ll) || repeats_ll < 1 || repeats_ll > 10000) throw std::runtime_error("invalid repetitions");\n    const int repeats = static_cast<int>(repeats_ll);\n    constexpr int rows = 1797, features = 64, classes = 10;\n    std::vector<double> raw(static_cast<std::size_t>(rows) * features);\n    for (double& value : raw) if (!(std::cin >> value) || !std::isfinite(value)) throw std::runtime_error("invalid input");\n    double extra = 0.0;\n    if (std::cin >> extra) throw std::runtime_error("trailing input");\n    std::array<int, rows> predictions{{}};\n    long long checksum = 0;\n    for (int iteration = 0; iteration < repeats; ++iteration) {{\n      const int shift = iteration % rows;\n      for (int row = 0; row < rows; ++row) {{\n        int input_row = row + shift;\n        if (input_row >= rows) input_row -= rows;\n        int best = 0; double maximum = -1000000.0;\n        for (int label = 0; label < classes; ++label) {{\n          double score = bias[static_cast<std::size_t>(label)];\n          for (int feature = 0; feature < features; ++feature) {{\n            const double value = raw[static_cast<std::size_t>(input_row) * features + feature] / 16.0;\n            score += value * weights[static_cast<std::size_t>(feature) * classes + label];\n          }}\n          if (score > maximum) {{ maximum = score; best = label; }}\n        }}\n        predictions[static_cast<std::size_t>(row)] = best; checksum += best;\n      }}\n    }}\n    for (int value : predictions) std::cout << value << '\\n';\n    std::cout << checksum << '\\n';\n    return 0;\n  }} catch (const std::exception& error) {{ std::cerr << error.what() << '\\n'; return 1; }}\n}}\n''')


def _build_commands(plan, inputs, out, compiler, clang):
    build = out / 'build'
    build.mkdir()
    source = inputs / 'source'
    ir = build / 'shorthand.ll'
    shorthand = build / 'shorthand.bin'
    cpp = build / 'cpp17-o3.bin'
    cpp_source = build / 'cpp17-o3.cpp'
    _write_cpp_source(source / 'model.json', cpp_source)
    build_reports = []
    build_reports.append(campaign.command([compiler, source / 'classifier.short', 'compile-mlir', '--output', ir], build, 'shorthand-ir'))
    build_reports.append(campaign.command([clang, ir, '-O2', '-fno-fast-math', '-o', shorthand], build, 'shorthand-link'))
    build_reports.append(campaign.command([clang, cpp_source, '-std=c++17', '-O3', '-fno-fast-math', '-o', cpp], build, 'cpp17-o3-build'))
    commands = {
        'shorthand': [shorthand],
        'numpy': [sys.executable, HERE / 'source_workload.py', '--model', source / 'model.json', '--baseline', 'numpy'],
        'cpp17-o3': [cpp],
    }
    if plan['profile'] == 'full':
        torch_python = plan['torch']['python']
        commands['pytorch-eager'] = [torch_python, HERE / 'source_workload.py', '--model', source / 'model.json', '--baseline', 'torch-eager']
        commands['pytorch-compile'] = [torch_python, HERE / 'source_workload.py', '--model', source / 'model.json', '--baseline', 'torch-compile']
        for name in ('rust-candle', 'mojo-max'):
            commands[name] = [plan['external'][name]['path'], '--model', source / 'model.json']
    return commands, build_reports, shorthand, cpp


def _verify_external_identities(plan):
    if plan['profile'] != 'full':
        return
    torch_python = pathlib.Path(plan['torch']['python'])
    if campaign.sha(torch_python) != plan['torch']['sha256'] or _tool_version(torch_python) != plan['torch']['versions']:
        raise ValueError('PyTorch environment changed after plan freeze')
    for name, spec in plan['external'].items():
        path = pathlib.Path(spec['path'])
        if campaign.sha(path) != spec['sha256']:
            raise ValueError(name + ' executable changed after plan freeze')
        probe = subprocess.run([str(path), '--version'], text=True, capture_output=True, timeout=60)
        if probe.returncode or probe.stdout.strip() != spec['version']:
            raise ValueError(name + ' version identity changed after plan freeze')


def run(args):
    plan_path = args.plan.resolve()
    if campaign.sha(plan_path) != args.plan_sha256:
        raise ValueError('predeclared state-of-practice plan digest mismatch')
    plan = campaign.load(plan_path)
    validate_plan(plan)
    campaign.verify_files(plan_path.parent, plan['hashes'])
    _verify_external_identities(plan)
    if platform.system() != 'Linux' or platform.machine() != 'x86_64':
        raise ValueError('declared comparison scope requires Linux x64')
    measured = plan['mode'] == 'calibrated_energy'
    compiler = pathlib.Path(shutil.which(str(args.compiler)) or args.compiler).absolute()
    clang = pathlib.Path(shutil.which(str(args.clang)) or args.clang).absolute()
    tool = pathlib.Path(shutil.which(str(args.tool)) or args.tool).absolute()
    identities = {str(p): campaign.sha(p) for p in (compiler, clang, tool, pathlib.Path(sys.executable))}
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
        policy = campaign.load(inputs / 'policy.json')
        campaign.validate_policy(policy)
        if (policy['mode'] == 'calibrated_energy') != measured:
            raise ValueError('policy mode mismatch')
        torch_cache = out / 'torch-inductor-cache'
        if plan['profile'] == 'full':
            torch_cache.mkdir()
            os.environ['TORCHINDUCTOR_CACHE_DIR'] = str(torch_cache)
        commands, build_reports, shorthand, cpp = _build_commands(plan, inputs, out, compiler, clang)
        expected = campaign.load(inputs / 'source/expected.json')
        warmups = []
        for n in range(plan['warmups']):
            for runner in ('shorthand',) + tuple(plan['baselines']):
                name = f'warmup-{n}-{runner}'
                trial = campaign.command(commands[runner], out, name, expected['completed'], inputs / 'source/input.txt')
                campaign.check_output(out / trial['stdout'], expected)
                warmups.append(dict(runner=runner, trial=trial))
        blocks = []
        for baseline in plan['baselines']:
            orders = [['shorthand', baseline], [baseline, 'shorthand']] * (plan['pairs'] // 2)
            random.Random(plan['seed'] + sum(map(ord, baseline))).shuffle(orders)
            for index, order in enumerate(orders):
                blocks.append(dict(baseline=baseline, index=index, order=order))
        random.Random(plan['seed']).shuffle(blocks)
        pairs = []
        for serial, block in enumerate(blocks):
            pair = dict(baseline=block['baseline'], index=block['index'], order=block['order'])
            for runner in block['order']:
                key = 'shorthand' if runner == 'shorthand' else 'baseline'
                name = f'pair-{serial}-{block["baseline"]}-{runner}'
                trial = campaign.command(commands[runner], out, name, expected['completed'], inputs / 'source/input.txt')
                campaign.check_output(out / trial['stdout'], expected)
                pair[key] = trial
            pairs.append(pair)
        meter = None
        if measured:
            time.sleep(1.0)
            meter = out / 'meter.csv'
            campaign.snapshot(pathlib.Path(plan['meter_csv']), meter)
            if not meter.read_bytes().endswith(b'\n'):
                raise ValueError('incomplete meter trace')
            all_trials = [p[k] for p in pairs for k in ('shorthand', 'baseline')]
            campaign.attach_energy(str(tool), out, all_trials, meter, inputs / 'instrument.json', policy)
        environment = dict(platform=platform.platform(), python=sys.version,
            clang_version=subprocess.check_output([clang, '--version'], text=True),
            executable_sha256=identities, source_binary_sha256=campaign.sha(shorthand), cpp_binary_sha256=campaign.sha(cpp),
            torch=plan.get('torch'), external=plan.get('external'),
            torchinductor_cache=str(torch_cache) if plan['profile'] == 'full' else None,
            started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
        campaign.write(out / 'environment.json', environment)
        report = dict(schema='shorthand.energy.state-of-practice.capture.v1', success=True,
            profile=plan['profile'], mode=plan['mode'], expected=expected,
            build_reports=build_reports, warmups=warmups, pairs=pairs,
            baseline_contract='identical FP64 model/input/repetitions/predictions/checksum; whole process warm-OS-cache window',
            meter_file='meter.csv' if meter else None, **CLAIMS)
        campaign.write(out / 'capture.json', report)
        _verify_external_identities(plan)
        campaign.verify_files(plan_path.parent, plan['hashes'])
        if campaign.sha(plan_path) != args.plan_sha256 or any(campaign.sha(pathlib.Path(p)) != digest for p, digest in identities.items()):
            raise ValueError('plan or experiment executable changed during capture')
        code_files = [HERE / 'state_of_practice.py', HERE / 'source_workload.py', HERE / 'analysis.py']
        campaign.write(out / 'code.json', {str(p.relative_to(ROOT)): campaign.sha(p) for p in code_files})
        if plan['profile'] == 'full' and torch_cache.exists():
            shutil.rmtree(torch_cache)
        hashes = {str(p.relative_to(out)): campaign.sha(p) for p in sorted(out.rglob('*')) if p.is_file()}
        manifest = dict(schema='shorthand.energy.state-of-practice.bundle.v1', success=True,
                        plan_sha256=args.plan_sha256, hashes=hashes, **CLAIMS)
        campaign.write(out / 'manifest.json', manifest)
        analyze(out, campaign.sha(out / 'manifest.json'), str(tool))
        print(f'bundle={out}\nmanifest_sha256={campaign.sha(out / "manifest.json")}')
    except Exception as error:
        campaign.write(out / 'failure.json', dict(success=False, error=str(error), **CLAIMS))
        raise


def analyze(out, expected_sha, tool):
    out = pathlib.Path(out).resolve()
    if campaign.sha(out / 'manifest.json') != expected_sha:
        raise ValueError('trusted state-of-practice bundle digest mismatch')
    manifest = campaign.load(out / 'manifest.json')
    if manifest.get('schema') != 'shorthand.energy.state-of-practice.bundle.v1' or manifest.get('success') is not True:
        raise ValueError('complete state-of-practice bundle required')
    campaign.verify_files(out, manifest['hashes'])
    plan = campaign.load(out / 'plan.json')
    validate_plan(plan)
    if campaign.sha(out / 'plan.json') != manifest['plan_sha256']:
        raise ValueError('plan integrity failure')
    report = campaign.load(out / 'capture.json')
    expected = campaign.load(out / 'inputs/source/expected.json')
    measured = plan['mode'] == 'calibrated_energy'
    cells = []
    uncertainty = campaign.load(out / 'inputs/instrument.json')['uncertainty_percent'] if measured else 0
    policy = campaign.load(out / 'inputs/policy.json')
    for baseline in plan['baselines']:
        pairs = [p for p in report['pairs'] if p['baseline'] == baseline]
        if len(pairs) != plan['pairs']:
            raise ValueError('missing comparison pairs for ' + baseline)
        orders = [p['order'] for p in pairs]
        if orders.count(['shorthand', baseline]) != plan['pairs'] // 2 or orders.count([baseline, 'shorthand']) != plan['pairs'] // 2:
            raise ValueError('unbalanced runner order for ' + baseline)
        shorthand_values, baseline_values = [], []
        for pair in pairs:
            for key in ('shorthand', 'baseline'):
                trial = pair[key]
                if trial['returncode'] != 0 or trial['completed'] != expected['completed']:
                    raise ValueError('failed or incomplete trial')
                campaign.check_output(out / trial['stdout'], expected)
                value = trial['elapsed_ms'] / trial['completed']
                if measured:
                    recorded = trial.get('energy')
                    if not recorded:
                        raise ValueError('missing measured energy for ' + baseline)
                    with tempfile.TemporaryDirectory() as temp:
                        scratch = pathlib.Path(temp)
                        trial_copy = dict(trial)
                        trial_copy.pop('energy', None)
                        campaign.attach_energy(tool, scratch, [trial_copy], out / 'meter.csv', out / 'inputs/instrument.json', policy)
                        if trial_copy['energy'] != recorded:
                            raise ValueError('physical-meter replay mismatch for ' + baseline)
                    value = recorded['joules_per_fu']
                (shorthand_values if key == 'shorthand' else baseline_values).append(value)
        for values in (shorthand_values, baseline_values):
            if any(not math.isfinite(v) or v <= 0 for v in values):
                raise ValueError('invalid observations')
            if measured:
                variability = 100 * statistics.stdev(values) / statistics.mean(values)
                if uncertainty + 2 * variability > policy['maximum_uncertainty_percent']:
                    raise ValueError('energy uncertainty exceeds policy for ' + baseline)
        comparison = campaign.energy_statistics.compare(shorthand_values, baseline_values,
                        seed=plan['seed'], uncertainty_percent=uncertainty if measured else 0)
        cells.append(dict(baseline=baseline, metric='joules_per_image' if measured else 'ms_per_image', comparison=comparison))
    summary = dict(schema='shorthand.energy.state-of-practice.summary.v1', profile=plan['profile'],
                   mode=plan['mode'], scope=plan['scope'], baselines=plan['baselines'],
                   full_matrix_qualified=(plan['profile'] == 'full' and tuple(plan['baselines']) == FULL_BASELINES),
                   energy_evidence_qualified=measured, cells=cells, **CLAIMS)
    campaign.write(out / 'sota-summary.json', summary)
    lines = ['# State-of-practice source benchmark', '',
             f'Profile: {plan["profile"]}. Mode: {plan["mode"]}.',
             'Each row is Shorthand versus the named baseline on the identical FP64 workload.',
             'No row is merged with the separate FP32 ONNX runtime track.', '',
             '| Baseline | Metric | Shorthand mean | Baseline mean | Shorthand reduction % | Paired 95% interval % |',
             '| --- | --- | ---: | ---: | ---: | --- |']
    for cell in cells:
        c = cell['comparison']
        lines.append(f'| {cell["baseline"]} | {cell["metric"]} | {c["native_mean"]:.6g} | {c["python_mean"]:.6g} | {c["savings_percent"]:.3f} | {c["paired_bootstrap_95_percent_interval"]} |')
    lines += ['', 'Negative reduction means Shorthand consumed more of the measured quantity.',
              'A full profile requires NumPy, optimized C++17, PyTorch eager, torch.compile, Rust/Candle and Mojo/MAX.',
              'The matrix is still a small CPU-classification study and cannot by itself establish a universal language or carbon claim.']
    (out / 'sota-summary.md').write_text('\n'.join(lines) + '\n')
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='action', required=True)
    prep = commands.add_parser('prepare')
    prep.add_argument('--output', type=pathlib.Path, required=True)
    prep.add_argument('--mode', choices=['execution_only', 'calibrated_energy'], required=True)
    prep.add_argument('--profile', choices=['core', 'full'], default='core')
    prep.add_argument('--repetitions', type=int, default=10)
    prep.add_argument('--pairs', type=int, default=30)
    prep.add_argument('--instrument', type=pathlib.Path)
    prep.add_argument('--meter-csv', type=pathlib.Path)
    prep.add_argument('--pytorch-python')
    prep.add_argument('--rust-command')
    prep.add_argument('--rust-version')
    prep.add_argument('--mojo-command')
    prep.add_argument('--mojo-version')
    capture = commands.add_parser('run')
    capture.add_argument('--plan', type=pathlib.Path, required=True)
    capture.add_argument('--plan-sha256', required=True)
    capture.add_argument('--compiler', required=True)
    capture.add_argument('--clang', default='clang++-18')
    capture.add_argument('--tool', required=True)
    capture.add_argument('--output', type=pathlib.Path, required=True)
    replay = commands.add_parser('analyze')
    replay.add_argument('--bundle', type=pathlib.Path, required=True)
    replay.add_argument('--manifest-sha256', required=True)
    replay.add_argument('--tool', required=True)
    args = parser.parse_args()
    if args.action == 'prepare':
        prepare(args)
    elif args.action == 'run':
        run(args)
    else:
        analyze(args.bundle, args.manifest_sha256, args.tool)


if __name__ == '__main__':
    main()
