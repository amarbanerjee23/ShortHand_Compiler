#!/usr/bin/env python3
"""Prepare, execute and replay bounded energy experiments. No estimated joules."""
import argparse
import contextlib
import datetime
import importlib.util
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

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_ai_application_baselines import load, sha, write, execute
from assess_ai_application_comparison import assess_bundle, validate_measurement, validate_policy
from create_digit_application_fixture import create
from source_workload import prepare as prepare_source

# Explicit module name avoids shadowing Python's standard statistics module.
spec = importlib.util.spec_from_file_location('energy_statistics', pathlib.Path(__file__).with_name('analysis.py'))
energy_statistics = importlib.util.module_from_spec(spec)
spec.loader.exec_module(energy_statistics)

ENV = dict(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
           BLIS_NUM_THREADS='1', VECLIB_MAXIMUM_THREADS='1', NUMEXPR_NUM_THREADS='1',
           PYTHONHASHSEED='0', ORT_DISABLE_TELEMETRY='1')
CLAIMS = dict(comparative_energy_claim=False, lowest_carbon_language_claim=False,
              official_certification_granted=False)


def bounded(value, low, high, name):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f'{name} must be an integer in [{low}, {high}]')


def snapshot(path, output):
    if path.is_symlink() or not path.is_file() or path.stat().st_size > 32 * 1024 * 1024:
        raise ValueError('unsafe snapshot input: ' + str(path))
    with path.open('rb') as stream:
        data = stream.read(32 * 1024 * 1024 + 1)
    if len(data) > 32 * 1024 * 1024:
        raise ValueError('snapshot grew beyond limit')
    output.write_bytes(data)


def verify_files(root, hashes):
    for name, digest in hashes.items():
        path = root / name
        if pathlib.PurePath(name).is_absolute() or '..' in pathlib.PurePath(name).parts:
            raise ValueError('unsafe artifact path')
        if not path.resolve().is_relative_to(root.resolve()) or path.is_symlink() or not path.is_file() or sha(path) != digest:
            raise ValueError('artifact changed or missing: ' + name)


def validate_plan(plan):
    if plan['schema'] != 'shorthand.energy.experiment.plan.v1' or plan['mode'] not in ('calibrated_energy', 'execution_only'):
        raise ValueError('invalid experiment plan')
    bounded(plan['source_pairs'], 4, 100, 'source pairs')
    bounded(plan['runtime_pairs'], 4, 10, 'runtime pairs')
    bounded(plan['compile_repetitions'], 1, 100, 'compile repetitions')
    if plan['source_pairs'] % 2 or plan['runtime_pairs'] % 2 or plan['compile_blocks'] != 3 or plan['warmups'] != 2:
        raise ValueError('incomplete fixed experimental design')
    if plan['source_baseline'] not in ('numpy', 'scalar'):
        raise ValueError('invalid Python baseline')
    expected = {'ort-b1-t1', 'ort-b16-t1', 'ort-b32-t1', 'ort-b16-t2', 'ort-b16-t4'}
    if len(plan['runtime_cases']) != 5 or {c['id'] for c in plan['runtime_cases']} != expected:
        raise ValueError('all five predefined runtime cells are required')
    if plan['mode'] == 'calibrated_energy' and (plan['source_pairs'] < 30 or not plan['meter_csv']):
        raise ValueError('measured campaign requires at least 30 source pairs and physical telemetry')
    if any(plan[key] is not value for key, value in CLAIMS.items()):
        raise ValueError('experiment plan cannot authorize claims')


def prepare(args):
    out = args.output.resolve()
    bounded(args.source_repetitions, 1, 10000, 'source repetitions')
    bounded(args.runtime_repetitions, 1, 10000, 'runtime repetitions')
    bounded(args.source_pairs, 4, 100, 'source pairs')
    bounded(args.runtime_pairs, 4, 10, 'runtime pairs')
    bounded(args.compile_repetitions, 1, 100, 'compile repetitions')
    if args.source_pairs % 2 or args.runtime_pairs % 2:
        raise ValueError('pair counts must be even')
    measured = args.mode == 'calibrated_energy'
    if measured and (not args.instrument or not args.meter_csv or args.source_pairs < 30):
        raise ValueError('measurement needs instrument, live meter CSV and at least 30 source pairs')
    if not measured and (args.instrument or args.meter_csv):
        raise ValueError('execution-only mode cannot accept energy evidence')
    out.mkdir(parents=True, exist_ok=False)
    prepare_source(out / 'source', args.source_repetitions)
    policy = load(ROOT / 'tests/ai_application' / (
        'comparison_measurement_policy.json' if measured else 'comparison_execution_policy.json'))
    write(out / 'policy.json', policy)
    if measured:
        snapshot(args.instrument, out / 'instrument.json')
    cases = []
    # One-factor sweeps: isolate batching at one thread, then threading at batch 16.
    for batch, threads in [(1, 1), (16, 1), (32, 1), (16, 2), (16, 4)]:
        name = f'ort-b{batch}-t{threads}'
        cpath = create(out / name, batch=batch, threads=threads)
        app = load(cpath)
        qpath = pathlib.Path(app['qualification_config'])
        q = load(qpath)
        q['repetitions'] = args.runtime_repetitions
        if measured:
            q.update(energy_source='physical_meter', require_measured_energy=True,
                     meter_csv=str(args.meter_csv.resolve()), instrument=load(out / 'instrument.json'))
        write(qpath, q)
        app['qualification_sha256'] = sha(qpath)
        write(cpath, app)
        cases.append(dict(id=name, application=str(cpath)))
    hashes = {str(p.relative_to(out)): sha(p) for p in sorted(out.rglob('*')) if p.is_file()}
    plan = dict(schema='shorthand.energy.experiment.plan.v1', mode=args.mode, seed=104,
                source_pairs=args.source_pairs, runtime_pairs=args.runtime_pairs,
                source_baseline=args.source_baseline, compile_repetitions=args.compile_repetitions,
                compile_blocks=3, warmups=2, runtime_cases=cases, hashes=hashes,
                meter_csv=str(args.meter_csv.resolve()) if measured else None,
                data_hashes={str(p.relative_to(ROOT)): sha(p) for p in
                             (ROOT / 'tests/ai_application/data').glob('*.csv')},
                scope='UCI Optdigits CPU classification only', **CLAIMS)
    validate_plan(plan)
    write(out / 'plan.json', plan)
    print(f'plan={out / "plan.json"}\nplan_sha256={sha(out / "plan.json")}')


def command(argv, directory, name, completed=1, stdin=None):
    """Whole subprocess window includes startup/imports/I/O and process teardown."""
    argv = list(map(str, argv))
    with contextlib.ExitStack() as stack:
        inp = stack.enter_context(stdin.open('rb')) if stdin else subprocess.DEVNULL
        stdout = stack.enter_context((directory / f'{name}.stdout').open('wb'))
        stderr = stack.enter_context((directory / f'{name}.stderr').open('wb'))
        start, monotonic = time.time(), time.perf_counter()
        try:
            result = subprocess.run(argv, stdin=inp, stdout=stdout, stderr=stderr,
                                    env=dict(os.environ, **ENV), timeout=1800, check=False)
        except subprocess.TimeoutExpired:
            write(directory / f'{name}.failure.json', dict(error='timeout', argv=argv))
            raise
        elapsed, end = time.perf_counter() - monotonic, time.time()
    trial = dict(argv=argv, completed=completed, start_unix_seconds=start, end_unix_seconds=end,
                 elapsed_ms=elapsed * 1000, returncode=result.returncode,
                 stdout=f'{name}.stdout', stderr=f'{name}.stderr')
    write(directory / f'{name}.json', trial)
    errors = (directory / f'{name}.stderr').read_text(errors='replace')
    if result.returncode or abs(end - start - elapsed) > .01:
        output = (directory / f'{name}.stdout').read_text(errors='replace')
        raise ValueError(f'failed command or discontinuous clock: {name}, exit={result.returncode}\n{errors[-4000:]}\n{output[-4000:]}')
    if any(v in errors for v in ('AddressSanitizer', 'LeakSanitizer', 'runtime error:')):
        raise ValueError('sanitizer finding in ' + name)
    return trial


def check_output(path, expected):
    values = [float(v) for v in path.read_text().split()]
    if values != expected['predictions'] + [expected['checksum']]:
        raise ValueError('full predictions or repetition checksum mismatch: ' + str(path))


def attach_energy(tool, directory, trials, trace, instrument, policy):
    trace_hash = sha(trace)
    for n, trial in enumerate(trials):
        target = directory / f'measurement-{n}.json'
        execute([tool, 'meter-window', trace, instrument, trial['start_unix_seconds'],
                 trial['end_unix_seconds'], trial['completed'], target])
        measured = load(target)
        validate_measurement(trial, measured, policy, trace_hash)
        if measured['functional_units'] != trial['completed'] or not math.isclose(
                measured['elapsed_seconds'] * 1000, trial['elapsed_ms'], abs_tol=10):
            raise ValueError('measurement units or clocks mismatch')
        trial['energy'] = measured


def source_experiment(plan, inputs, out, compiler, clang):
    out.mkdir()
    source = inputs / 'source'
    expected = load(source / 'expected.json')
    compilations = []
    binary = None
    for block in range(plan['compile_blocks']):
        start, monotonic = time.time(), time.perf_counter()
        for iteration in range(plan['compile_repetitions']):
            name = f'build-{block}-{iteration}'
            ir, binary = out / f'{name}.ll', out / f'{name}.bin'
            command([compiler, source / 'classifier.short', 'compile-mlir', '--output', ir], out, name + '-ir')
            command([clang, ir, '-O2', '-fno-fast-math', '-o', binary], out, name + '-link')
        elapsed, end = time.perf_counter() - monotonic, time.time()
        if abs(end - start - elapsed) > .01:
            raise ValueError('compilation clock discontinuity')
        compilations.append(dict(start_unix_seconds=start, end_unix_seconds=end,
                                 elapsed_ms=elapsed * 1000, completed=plan['compile_repetitions']))
    cmds = dict(native=[binary], python=[sys.executable, pathlib.Path(__file__).with_name('source_workload.py'),
                '--model', source / 'model.json', '--baseline', plan['source_baseline']])
    for n in range(plan['warmups']):
        for runner in cmds:
            name = f'warmup-{n}-{runner}'
            command(cmds[runner], out, name, expected['completed'], source / 'input.txt')
            check_output(out / f'{name}.stdout', expected)
    orders = [['native', 'python'], ['python', 'native']] * (plan['source_pairs'] // 2)
    random.Random(plan['seed']).shuffle(orders)
    pairs = []
    for n, order in enumerate(orders):
        pair = dict(order=order)
        for runner in order:
            name = f'pair-{n}-{runner}'
            pair[runner] = command(cmds[runner], out, name, expected['completed'], source / 'input.txt')
            check_output(out / f'{name}.stdout', expected)
        pairs.append(pair)
    report = dict(boundary='whole_process_warm_os_cache', precision='float64',
                  baseline=plan['source_baseline'], functional_unit='completed_image',
                  expected=expected, binary_sha256=sha(binary), compilations=compilations, pairs=pairs)
    write(out / 'source.json', report)
    return report


def run(args):
    plan_path = args.plan.resolve()
    if sha(plan_path) != args.plan_sha256:
        raise ValueError('predeclared plan digest mismatch')
    plan = load(plan_path)
    validate_plan(plan)
    verify_files(plan_path.parent, plan['hashes'])
    verify_files(ROOT, plan['data_hashes'])
    for key, value in CLAIMS.items():
        if plan[key] is not value:
            raise ValueError('experiment plan cannot authorize claims')
    measured = plan['mode'] == 'calibrated_energy'
    if plan['mode'] not in ('calibrated_energy', 'execution_only'):
        raise ValueError('invalid experiment mode')
    if measured and plan['source_pairs'] < 30:
        raise ValueError('at least 30 source pairs required for physical experiments')
    available = len(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else os.cpu_count()
    if measured and (not available or available < 4):
        raise ValueError('physical campaign requires four available CPU threads; no cells skipped')
    if platform.system() != 'Linux' or platform.machine() != 'x86_64':
        raise ValueError('declared experiment scope requires Linux x64')
    # Preserve the invoked symlink name: resolving venv/bin/python bypasses
    # its installed baseline packages; resolving clang++ changes driver mode.
    executables = [pathlib.Path(shutil.which(str(p)) or p).absolute() for p in (args.compiler, args.clang, args.tool, sys.executable)]
    compiler, clang, tool, python = executables
    identities = {str(p): sha(p) for p in executables}
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    try:
        inputs = out / 'inputs'
        inputs.mkdir()
        for name in plan['hashes']:
            target = inputs / name
            target.parent.mkdir(parents=True, exist_ok=True)
            snapshot(plan_path.parent / name, target)
        verify_files(inputs, plan['hashes'])
        snapshot(plan_path, out / 'plan.json')
        policy = load(inputs / 'policy.json')
        validate_policy(policy)
        if (policy['mode'] == 'calibrated_energy') != measured:
            raise ValueError('policy mode mismatch')
        revision = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True, capture_output=True)
        write(out / 'environment.json', dict(platform=platform.platform(), python=sys.version,
            available_cpus=available, affinity=sorted(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else None,
            cpuinfo=pathlib.Path('/proc/cpuinfo').read_text() if pathlib.Path('/proc/cpuinfo').exists() else None,
            compiler_revision=revision.stdout.strip() if revision.returncode == 0 else None,
            executable_sha256=identities, thread_environment=ENV,
            clang_version=subprocess.check_output([clang, '--version'], text=True),
            started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat()))
        code = {str(p.relative_to(ROOT)): sha(p) for p in [pathlib.Path(__file__),
                pathlib.Path(__file__).with_name('source_workload.py'), pathlib.Path(__file__).with_name('analysis.py'),
                ROOT / 'scripts/compare_ai_application_baselines.py', ROOT / 'scripts/assess_ai_application_comparison.py']}
        write(out / 'code.json', code)
        source = source_experiment(plan, inputs, out / 'source', compiler, clang)
        # Freeze one trace after ALL source/compile windows. Never integrate live bytes twice.
        if measured:
            # Allow the independent logger to flush a bracketing sample; this
            # acquisition delay is outside every measurement window.
            time.sleep(1.0)
            snapshot(pathlib.Path(plan['meter_csv']), out / 'source/meter.csv')
            if not (out / 'source/meter.csv').read_bytes().endswith(b'\n'):
                raise ValueError('incomplete meter trace')
            trials = source['compilations'] + [p[k] for p in source['pairs'] for k in p['order']]
            attach_energy(tool, out / 'source', trials, out / 'source/meter.csv', inputs / 'instrument.json', policy)
            write(out / 'source/source.json', source)
        # Existing v2 harness and assessor remain the authority for FP32 ORT comparisons.
        cases = list(plan['runtime_cases'])
        random.Random(plan['seed']).shuffle(cases)
        for case in cases:
            target = out / case['id']
            argv = [python, ROOT / 'scripts/compare_ai_application_baselines.py', '--tool', tool,
                    '--config', case['application'], '--policy', inputs / 'policy.json',
                    '--pairs', plan['runtime_pairs'], '--output', target]
            if measured:
                argv.append('--require-energy')
            execute(argv)
            assessment = assess_bundle(target, sha(target / 'comparison.json'), sha(inputs / 'policy.json'), str(tool))
            write(target / 'assessment.json', assessment)
            if not assessment['success']:
                raise ValueError('runtime assessment failed: ' + case['id'])
        verify_files(plan_path.parent, plan['hashes'])
        verify_files(ROOT, plan['data_hashes'])
        verify_files(ROOT, code)
        if sha(plan_path) != args.plan_sha256 or any(sha(p) != digest for p, digest in identities.items()):
            raise ValueError('plan or executable changed during execution')
        # Intermediate binaries/IR are retained; hashes bind them with reports and raw outputs.
        hashes = {str(p.relative_to(out)): sha(p) for p in sorted(out.rglob('*')) if p.is_file()}
        manifest = dict(schema='shorthand.energy.experiment.bundle.v1', success=True,
                        plan_sha256=args.plan_sha256, hashes=hashes, **CLAIMS)
        write(out / 'manifest.json', manifest)
        analyze(out, sha(out / 'manifest.json'), str(tool))
        print(f'bundle={out}\nmanifest_sha256={sha(out / "manifest.json")}')
    except Exception as error:
        write(out / 'failure.json', dict(success=False, error=str(error), **CLAIMS))
        raise


def analyze(out, expected_sha, tool):
    if sha(out / 'manifest.json') != expected_sha:
        raise ValueError('trusted bundle digest mismatch')
    manifest = load(out / 'manifest.json')
    if manifest['schema'] != 'shorthand.energy.experiment.bundle.v1' or manifest['success'] is not True:
        raise ValueError('complete experiment bundle required')
    verify_files(out, manifest['hashes'])
    plan = load(out / 'plan.json')
    validate_plan(plan)
    if sha(out / 'plan.json') != manifest['plan_sha256']:
        raise ValueError('plan integrity failure')
    measured = plan['mode'] == 'calibrated_energy'
    policy = load(out / 'inputs/policy.json')
    validate_policy(policy)
    if (policy['mode'] == 'calibrated_energy') != measured:
        raise ValueError('replay policy mode mismatch')
    source = load(out / 'source/source.json')
    if len(source['compilations']) != plan['compile_blocks'] or any(
            t['completed'] != plan['compile_repetitions'] for t in source['compilations']):
        raise ValueError('incomplete compilation trials')
    expected = load(out / 'inputs/source/expected.json')
    orders = [p['order'] for p in source['pairs']]
    if len(orders) != plan['source_pairs'] or orders.count(['native', 'python']) != len(orders) // 2 or orders.count(['python', 'native']) != len(orders) // 2:
        raise ValueError('incomplete or unbalanced source pairs')
    previous_end = 0
    trials = source['compilations'] + [p[k] for p in source['pairs'] for k in p['order']]
    for trial in trials:
        if trial['start_unix_seconds'] < previous_end or trial['end_unix_seconds'] <= trial['start_unix_seconds']:
            raise ValueError('overlapping or reordered source windows')
        previous_end = trial['end_unix_seconds']
        if not math.isfinite(trial['elapsed_ms']) or trial['elapsed_ms'] <= 0 or abs(
                trial['end_unix_seconds'] - trial['start_unix_seconds'] - trial['elapsed_ms'] / 1000) > .01:
            raise ValueError('invalid source clock window')
    for pair in source['pairs']:
        for runner in ('native', 'python'):
            trial = pair[runner]
            if trial['returncode'] != 0 or trial['completed'] != expected['completed']:
                raise ValueError('failed or incomplete source trial')
            check_output(out / 'source' / trial['stdout'], expected)
    uncertainty = 0
    if measured:
        instrument = out / 'inputs/instrument.json'
        trace = out / 'source/meter.csv'
        with tempfile.TemporaryDirectory() as temp:
            scratch = pathlib.Path(temp)
            for trial in trials:
                recorded = trial['energy']
                attach_energy(tool, scratch, [dict(trial)], trace, instrument, policy)
                if load(scratch / 'measurement-0.json') != recorded:
                    raise ValueError('source physical-meter replay mismatch')
        uncertainty = load(instrument)['uncertainty_percent']
    timings = [[p[k]['elapsed_ms'] / p[k]['completed'] for p in source['pairs']] for k in ('native', 'python')]
    result = dict(schema='shorthand.energy.experiment.summary.v1', scope=plan['scope'],
                  energy_evidence_qualified=measured, source_baseline=source['baseline'], **CLAIMS,
                  source_ms_per_image=energy_statistics.compare(*timings, seed=plan['seed']),
                  source_energy=None, compilation_amortization=None, runtime_cells=[])
    if measured:
        energies = [[p[k]['energy']['joules_per_fu'] for p in source['pairs']] for k in ('native', 'python')]
        # Apply the same engineering stability limits used by the ORT assessor.
        # Preserve the entire failed session instead of filtering noisy trials.
        for values in energies + [[t['energy']['joules_per_fu'] for t in source['compilations']]]:
            variability = 100 * statistics.stdev(values) / statistics.mean(values)
            if uncertainty + 2 * variability > policy['maximum_uncertainty_percent']:
                raise ValueError('source or compilation energy uncertainty exceeds declared policy')
        for values in timings:
            if 100 * statistics.stdev(values) / statistics.mean(values) > policy['maximum_trial_variability_percent']:
                raise ValueError('source timing variability exceeds declared policy')
        result['source_energy'] = energy_statistics.compare(*energies, seed=plan['seed'], uncertainty_percent=uncertainty)
        result['compilation_amortization'] = energy_statistics.break_even(
            statistics.mean(t['energy']['joules_per_fu'] for t in source['compilations']),
            statistics.mean(energies[0]) * expected['completed'], statistics.mean(energies[1]) * expected['completed'], expected['completed'])
    for case in plan['runtime_cases']:
        directory = out / case['id']
        assessment = assess_bundle(directory, sha(directory / 'comparison.json'), sha(out / 'inputs/policy.json'), tool)
        if not assessment['success']:
            raise ValueError('runtime replay assessment failed')
        comparison = load(directory / 'comparison.json')
        a, b = [], []
        for pair in comparison['pairs']:
            for key, values in [('native', a), ('python', b)]:
                report = load(directory / pair[key + '_file'])
                # Resample process-pairs, never the correlated inner trials as independent runs.
                values.append(statistics.mean(t['energy']['joules_per_fu'] if measured else
                    t['elapsed_ms'] / t['completed'] for t in report['trials']))
        cell_uncertainty = load(directory / 'instrument.json')['uncertainty_percent'] if measured else 0
        result['runtime_cells'].append(dict(id=case['id'], metric='joules_per_image' if measured else 'ms_per_image',
            comparison=energy_statistics.compare(a, b, seed=plan['seed'], uncertainty_percent=cell_uncertainty)))
    write(out / 'summary.json', result)
    lines = ['# Optdigits energy experiments', '', f'Mode: {plan["mode"]}. Scope: {plan["scope"]}.', '',
             'All results are within-session observations. Repeat the frozen plan in three independent sessions.',
             'No overall language, carbon or certification claim is authorized.', '',
             '| Experiment | Metric | Shorthand mean | Python mean | Observed reduction % | Paired 95% interval % |',
             '| --- | --- | ---: | ---: | ---: | --- |']
    rows = [('Compiled FP64', 'J/image' if measured else 'ms/image', result['source_energy'] if measured else result['source_ms_per_image'])]
    rows += [(c['id'], c['metric'], c['comparison']) for c in result['runtime_cells']]
    for name, metric, c in rows:
        lines.append(f'| {name} | {metric} | {c["native_mean"]:.6g} | {c["python_mean"]:.6g} | {c["savings_percent"]:.3f} | {c["paired_bootstrap_95_percent_interval"]} |')
    lines += ['', 'Negative reduction means Shorthand used more of the measured quantity.',
              'Instrument-expanded ranges and compilation break-even are retained in summary.json.',
              'Timing-only results cannot quantify energy savings. Intervals are exploratory, not simultaneous multi-workload claims.']
    (out / 'summary.md').write_text('\n'.join(lines) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='action', required=True)
    prep = commands.add_parser('prepare')
    prep.add_argument('--output', type=pathlib.Path, required=True)
    prep.add_argument('--mode', choices=['execution_only', 'calibrated_energy'], required=True)
    prep.add_argument('--source-repetitions', type=int, default=10)
    prep.add_argument('--runtime-repetitions', type=int, default=10)
    prep.add_argument('--source-pairs', type=int, default=30)
    prep.add_argument('--runtime-pairs', type=int, default=10)
    prep.add_argument('--compile-repetitions', type=int, default=10)
    prep.add_argument('--source-baseline', choices=['numpy', 'scalar'], default='numpy')
    prep.add_argument('--instrument', type=pathlib.Path)
    prep.add_argument('--meter-csv', type=pathlib.Path)
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
        analyze(args.bundle.resolve(), args.manifest_sha256, args.tool)


if __name__ == '__main__':
    main()
