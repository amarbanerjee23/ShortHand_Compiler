#!/usr/bin/env python3
"""Balanced, replayable resident AIRuntime / C++ ORT / Python ORT observations.

Uses existing uninstrumented runners. No physical energy or performance gate.
All five cells and all six runner permutations are mandatory.
"""
import argparse
import itertools
import math
import os
import pathlib
import platform
import random
import statistics
import subprocess
import sys

import campaign
import runtime_state_of_practice as cpp
from compare_ai_application_baselines import validate_pair

ROOT = pathlib.Path(__file__).resolve().parents[2]
RUNNERS = ('native', 'cpp_onnx', 'python_onnx')
CELLS = ((1, 1), (16, 1), (32, 1), (16, 2), (16, 4))
CLAIMS = dict(campaign.CLAIMS, production_claim=False, measured_energy_available=False, latency_claim_eligible=False)
SCHEMA = 'shorthand.energy.resident_baselines.v1'


def orders(seed, blocks):
    if type(blocks) is not int or blocks not in (6, 12):
        raise ValueError('six or twelve balanced blocks required')
    result = [list(p) for p in itertools.permutations(RUNNERS)] * (blocks // 6)
    random.Random(seed).shuffle(result)
    return result


def resources(path):
    fields = pathlib.Path(path).read_text().strip().split(',')
    if len(fields) != 3:
        raise ValueError('invalid GNU time resource record')
    user, system = map(float, fields[:2])
    rss = int(fields[2])
    if not all(math.isfinite(v) and v >= 0 for v in (user, system)) or rss <= 0:
        raise ValueError('invalid process resource measurement')
    return dict(process_cpu_ms=(user + system) * 1000, maximum_rss_bytes=rss * 1024)


def validate_classification(report):
    scores, predicted, top = report['scores'], report['predictions'], report.get('top_k')
    if len(scores) != 17970 or len(predicted) != 1797 or not isinstance(top, list) or len(top) != 5391:
        raise ValueError('incomplete scores, predictions or top-3')
    if any(type(v) not in (int, float) or not math.isfinite(v) for v in scores):
        raise ValueError('nonfinite classification score')
    if any(type(v) is not int or not 0 <= v < 10 for v in predicted + top):
        raise ValueError('invalid class label')
    expected = []
    for offset in range(0, len(scores), 10):
        expected.extend(sorted(range(10), key=lambda n: (-scores[offset + n], n))[:3])
    if top != expected or predicted != expected[::3]:
        raise ValueError('unstable or incorrect top-3')


def checked_process(directory, name):
    value = campaign.load(directory / (name + '.json'))
    if value.get('returncode') != 0 or value.get('completed') != 1797 * 2 * 3:
        raise ValueError('failed/incomplete process')
    elapsed = value.get('elapsed_ms')
    if type(elapsed) not in (int, float) or not math.isfinite(elapsed) or elapsed <= 0:
        raise ValueError('invalid process time')
    return dict(process_elapsed_ms=elapsed, **resources(directory / (name + '.resources.csv')))


def summarize(out, plan):
    if plan.get('schema') != SCHEMA or plan.get('runners') != list(RUNNERS):
        raise ValueError('unknown baseline design')
    if any(plan.get(key) is not value for key, value in CLAIMS.items()):
        raise ValueError('observations cannot authorize claims')
    if plan.get('cells') != [f'b{b}-t{t}' for b, t in CELLS]:
        raise ValueError('all five cells are required')
    if type(plan.get('seed')) is not int:
        raise ValueError('invalid randomization seed')
    rows = []
    for batch, threads in CELLS:
        cell = f'b{batch}-t{threads}'
        fixture = out / 'inputs' / cell
        app = campaign.load(fixture / 'application.json')
        q = campaign.load(fixture / 'qualification.json')
        if (q['batch_size'], app['threads'], q['repetitions'], q['trials'], q['warmups']) != (batch, threads, 2, 3, 2):
            raise ValueError('frozen workload changed')
        if q['energy_source'] != 'unavailable' or q.get('require_measured_energy', False):
            raise ValueError('this capture does not qualify energy')
        expected = dict(features=64, classes=10, top_k=3, input_min=0, input_max=16,
                        offset=0, scale=0.0625, minimum_accuracy=0.85)
        if any(app.get(key) != value for key, value in expected.items()) or q['input_shape'] != [batch, 64] or q['output_shape'] != [batch, 10]:
            raise ValueError('incompatible application contract')
        if (campaign.sha(fixture / 'digits.onnx') != q['model_sha256'] or
                campaign.sha(fixture / 'dataset.csv') != app['dataset_sha256'] or
                campaign.sha(fixture / 'qualification.json') != app['qualification_sha256']):
            raise ValueError('fixture integrity mismatch')
        sequence = orders(plan['seed'] + batch * 31 + threads, plan['blocks'])
        if plan['orders'].get(cell) != sequence:
            raise ValueError('unbalanced or altered runner order')
        observations = {name: [] for name in RUNNERS}
        reference = None
        for index, order in enumerate(sequence):
            directory = out / 'runs' / cell / str(index)
            if campaign.load(directory / 'order.json') != order:
                raise ValueError('incomplete/altered block')
            native = cpp.validate_native_report(directory / 'native-report.json', reference)
            python = cpp.validate_native_report(directory / 'python-report.json', native['predictions'])
            reference = native['predictions']
            validate_classification(native)
            validate_classification(python)
            validate_pair(native, python, q)
            if native['top_k'] != python['top_k']:
                raise ValueError('cross-runner top-3 mismatch')
            if (native['batch_size'], native['threads'], native['backend_version'], native['compiler_revision']) != (batch, threads, '1.30.0', plan['revision']):
                raise ValueError('runner revision or session mismatch')
            for key, value in [('configuration_sha256', campaign.sha(fixture / 'application.json')),
                               ('qualification_sha256', campaign.sha(fixture / 'qualification.json')),
                               ('model_sha256', q['model_sha256']), ('dataset_sha256', app['dataset_sha256'])]:
                if native[key] != value:
                    raise ValueError('runner used a different fixture: ' + key)
            if (python.get('numpy_version') != '2.3.5' or python.get('python_version', '').split('.')[:2] != ['3', '12'] or
                    python.get('baseline_source_sha256') != campaign.sha(out / 'implementation/python_worker.py')):
                raise ValueError('unpinned Python baseline')
            trial_path = directory / 'cpp-trials.csv'
            cpp.check_cpp_output(directory / 'cpp_onnx.stdout', native['predictions'], sum(native['predictions']) * 2 * 3)
            cpp.check_cpp_validation(trial_path, native)
            trials = {'native': native['trials'], 'python_onnx': python['trials'],
                      'cpp_onnx': cpp.load_cpp_trials(trial_path, q)}
            for name in order:
                metric = checked_process(directory, name)
                if metric['maximum_rss_bytes'] > q['maximum_memory_bytes']:
                    raise ValueError('baseline exceeded declared process memory limit')
                metric['resident_us_per_image'] = statistics.median(t['elapsed_ms'] * 1000 / t['completed'] for t in trials[name])
                observations[name].append(metric)
        for name in RUNNERS:
            values = observations[name]
            medians = {key: statistics.median(v[key] for v in values) for key in values[0]}
            rows.append(dict(cell=cell, runner=name, blocks=len(values), **medians))
    return dict(schema=SCHEMA, success=True, rows=rows, **CLAIMS)


def markdown(summary):
    lines = ['# Resident baseline observations', '',
             'One machine and session; descriptive block medians, not a speedup or energy claim.',
             'Resident time includes normalization, inference, validation, top-3 and harness work.',
             'Each runner retains its existing operational overhead; AIRuntime includes telemetry and batch timing.',
             'This compares application paths, not language syntax or model kernels in isolation.',
             'Process time includes startup, imports, model loading, warmup, all trials and report I/O.',
             'CPU and RSS use GNU time for each process, including child work; RSS is a maximum, not a sum.',
             'Resident microseconds/image is amortized dataset time, not request p95 latency.', '',
             '| Cell | Runner | Resident µs/image | Process ms | Process CPU ms | Maximum RSS MiB | Joules/image |',
             '| --- | --- | ---: | ---: | ---: | ---: | --- |']
    for row in summary['rows']:
        lines.append(f"| {row['cell']} | {row['runner']} | {row['resident_us_per_image']:.3f} | "
                     f"{row['process_elapsed_ms']:.3f} | {row['process_cpu_ms']:.3f} | "
                     f"{row['maximum_rss_bytes'] / 1048576:.3f} | Unavailable |")
    lines += ['', 'All six runner orders occur equally often in every cell. All completed blocks are retained.',
              'No timing threshold is enforced on hosted CI. Physical energy, C/Rust/Java controls and broader workloads remain pending.', '']
    return '\n'.join(lines)


def analyze(out, digest):
    if campaign.sha(out / 'manifest.json') != digest:
        raise ValueError('manifest digest mismatch')
    manifest = campaign.load(out / 'manifest.json')
    if manifest.get('schema') != SCHEMA or manifest.get('success') is not True:
        raise ValueError('unsuccessful capture')
    campaign.verify_files(out, manifest['hashes'])
    observed = {str(p.relative_to(out)) for p in out.rglob('*') if p.is_file() and p.name != 'manifest.json'}
    if observed != set(manifest['hashes']):
        raise ValueError('unrecorded or missing evidence')
    if campaign.sha(out / 'plan.json') != manifest['plan_sha256']:
        raise ValueError('frozen plan digest mismatch')
    plan = campaign.load(out / 'plan.json')
    campaign.verify_files(out, plan['input_hashes'])
    result = summarize(out, plan)
    if campaign.load(out / 'summary.json') != result or (out / 'BASELINES.md').read_text() != markdown(result):
        raise ValueError('summary is not replayable')
    return result


def run(args):
    orders(args.seed, args.blocks)
    if platform.system() != 'Linux' or sys.version_info[:2] != (3, 12):
        raise ValueError('this locked capture requires Linux CPython 3.12')
    tool, control = map(cpp.resolve_executable, (args.tool, args.cpp))
    python = pathlib.Path(sys.executable).absolute()  # Preserve the virtualenv executable path.
    timer = pathlib.Path('/usr/bin/time')
    timer_version = subprocess.check_output([timer, '--version'], text=True)
    if 'GNU' not in timer_version:
        raise ValueError('GNU time required for per-process CPU and RSS')
    if subprocess.check_output([control, '--version'], text=True).strip() != '1.30.0':
        raise ValueError('pinned C++ ONNX Runtime required')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    try:
        implementation = out / 'implementation'
        implementation.mkdir()
        sources = {'python_worker.py': ROOT / 'scripts/compare_ai_application_baselines.py',
                   'cpp_onnx_baseline.cpp': ROOT / 'experiments/energy/cpp_onnx_baseline.cpp',
                   'resident_baselines.py': pathlib.Path(__file__)}
        for name, path in sources.items():
            campaign.snapshot(path, implementation / name)
        revision = subprocess.check_output(['git', '-C', ROOT, 'rev-parse', 'HEAD'], text=True).strip()
        binary_hashes = {str(p): campaign.sha(p) for p in (tool, control, python, timer)}
        plan = dict(schema=SCHEMA, runners=list(RUNNERS), cells=[f'b{b}-t{t}' for b, t in CELLS],
                    blocks=args.blocks, seed=args.seed, revision=revision, binary_hashes=binary_hashes,
                    orders={}, **CLAIMS)
        for batch, threads in CELLS:
            cell = f'b{batch}-t{threads}'
            fixture = out / 'inputs' / cell
            app_path = campaign.create(fixture, batch=batch, threads=threads)
            app = campaign.load(app_path)
            campaign.snapshot(pathlib.Path(app['dataset_path']), fixture / 'dataset.csv')
            app['dataset_path'] = str(fixture / 'dataset.csv')
            campaign.write(app_path, app)
            cpp.case_inputs({'application': app_path, 'id': cell})
            plan['orders'][cell] = orders(args.seed + batch * 31 + threads, args.blocks)
        # Plan and all input bytes are frozen before the first measured block.
        frozen = {str(p.relative_to(out)): campaign.sha(p) for folder in ('inputs', 'implementation') for p in (out / folder).rglob('*') if p.is_file()}
        plan['input_hashes'] = frozen
        campaign.write(out / 'plan.json', plan)
        plan_digest = campaign.sha(out / 'plan.json')
        campaign.write(out / 'environment.json', dict(platform=platform.platform(), python=sys.version,
            thread_environment=campaign.ENV, gnu_time=timer_version,
            affinity=sorted(os.sched_getaffinity(0)), cpuinfo=pathlib.Path('/proc/cpuinfo').read_text(),
            python_packages=subprocess.check_output([python, '-m', 'pip', 'freeze'], text=True)))
        for batch, threads in CELLS:
            cell = f'b{batch}-t{threads}'
            app_path = out / 'inputs' / cell / 'application.json'
            app = campaign.load(app_path); q = campaign.load(out / 'inputs' / cell / 'qualification.json')
            for index, order in enumerate(plan['orders'][cell]):
                directory = out / 'runs' / cell / str(index)
                directory.mkdir(parents=True)
                campaign.write(directory / 'order.json', order)
                commands = {
                    'native': [tool, 'application', app_path, directory / 'native-report.json'],
                    'cpp_onnx': [control, q['model_path'], app['dataset_path'], batch, threads, 2, 2, 3, directory / 'cpp-trials.csv'],
                    'python_onnx': [python, implementation / 'python_worker.py', '--python-worker', '--tool', tool,
                                    '--config', app_path, '--output', directory / 'python-report.json'],
                }
                for name in order:
                    campaign.command([timer, '-f', '%U,%S,%M', '-o', directory / (name + '.resources.csv'), *commands[name]],
                                     directory, name, 1797 * 2 * 3)
        if campaign.sha(out / 'plan.json') != plan_digest or any(campaign.sha(path) != digest for path, digest in binary_hashes.items()):
            raise ValueError('plan or executable changed during capture')
        campaign.verify_files(out, frozen)
        result = summarize(out, plan)
        campaign.write(out / 'summary.json', result)
        (out / 'BASELINES.md').write_text(markdown(result))
        hashes = {str(p.relative_to(out)): campaign.sha(p) for p in out.rglob('*') if p.is_file()}
        campaign.write(out / 'manifest.json', dict(schema=SCHEMA, success=True, plan_sha256=plan_digest, hashes=hashes))
        digest = campaign.sha(out / 'manifest.json')
        analyze(out, digest)
        print(f'manifest_sha256={digest}')
    except Exception as error:
        campaign.write(out / 'failure.json', dict(success=False, error=str(error), **CLAIMS))
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='action', required=True)
    capture = commands.add_parser('run')
    capture.add_argument('--tool', required=True)
    capture.add_argument('--cpp', required=True)
    capture.add_argument('--output', type=pathlib.Path, required=True)
    capture.add_argument('--blocks', type=int, default=6)
    capture.add_argument('--seed', type=int, default=108)
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
