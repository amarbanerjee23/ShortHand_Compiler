#!/usr/bin/env python3
"""Replayable compiled .short FP32 smoke/workload evidence, separate from digits accuracy."""
import argparse
import hashlib
import json
import math
import os
import pathlib
import platform
import statistics
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from create_digit_application_fixture import create
from ci_energy_evidence import ComponentEnergySampler, cpu_identity

BOUNDARY = ('compiled .short entry including zero tensor initialization, registration, inference, '
            'runtime validation/telemetry/logging and independent output verification; direct ORT '
            'includes its prepared invocation and identical output verification; block wall clocks')
PHASES = ('entry_residual', 'facade_lock', 'registration', 'bridge_validation', 'descriptor_input',
          'policy_refresh', 'hardware_probe', 'routing', 'runtime_validation', 'snapshot',
          'session_prepare', 'backend_other', 'ort_run', 'runtime_telemetry', 'bridge_output',
          'bridge_telemetry_log', 'oracle')
PROFILE_ITERATIONS, PROFILE_BLOCKS = 32, 2


def configuration(runner):
    return dict(graph_optimization='all' if runner == 'direct_all' else 'basic',
                execution_mode='sequential', spinning=False, intra_threads=1, inter_threads=1)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, data):
    path.write_text(json.dumps(data, indent=2, sort_keys=True, allow_nan=False) + '\n')


def command(argv, out, stem):
    write(out / (stem + '.command.json'), list(map(str, argv)))
    with (out / (stem + '.stdout')).open('w') as stdout, (out / (stem + '.stderr')).open('w') as stderr:
        start = time.perf_counter()
        result = subprocess.run(list(map(str, argv)), stdout=stdout, stderr=stderr, timeout=240)
        elapsed = time.perf_counter() - start
    if result.returncode:
        raise ValueError(f'{stem} failed ({result.returncode}); see retained stderr')
    return elapsed


def validate_sample(sample, batch, iterations, blocks, runner='head', instrumented=False):
    counts = ('batch', 'threads', 'iterations', 'warmups', 'completed_calls', 'completed_vectors')
    if (not isinstance(sample, dict) or any(type(sample.get(k)) is not int for k in counts)
            or sample.get('schema') != 'shorthand.generated_infer.sample.v2' or sample.get('success') is not True
            or sample.get('batch') != batch or sample.get('threads') != 1
            or sample.get('iterations') != iterations or sample.get('warmups') != 8
            or sample.get('completed_calls') != iterations * blocks
            or sample.get('completed_vectors') != iterations * blocks * batch):
        raise ValueError('incomplete or mismatched generated sample')
    times = sample.get('block_elapsed_ms', [])
    if not isinstance(times, list) or len(times) != blocks or any(type(t) not in (float, int) or not math.isfinite(t) or t <= 0 for t in times):
        raise ValueError('invalid generated timings')
    if type(sample.get('cold_session_ms')) not in (int, float) or not math.isfinite(sample['cold_session_ms']) or sample['cold_session_ms'] <= 0:
        raise ValueError('invalid cold-session timing')
    config = sample.get('configuration')
    if (config != configuration(runner) or type(config.get('spinning')) is not bool
            or any(type(config.get(k)) is not int for k in ('intra_threads', 'inter_threads'))):
        raise ValueError('mismatched ORT control configuration')
    if sample.get('instrumented') is not instrumented:
        raise ValueError('instrumentation cannot enter latency or energy controls')
    if not instrumented and any(k in sample for k in ('phase_samples', 'clock_pair_median_ns')):
        raise ValueError('unexpected profiling data in uninstrumented sample')
    if runner in ('head', 'profiled'):
        cache = sample.get('last_runtime_telemetry', {}).get('ai_runtime_telemetry', {}).get('execution_evidence', {})
        if cache.get('hit') is not True or type(cache.get('preparations')) is not int or cache['preparations'] != 1:
            raise ValueError('generated path did not retain one prepared session')


def validate_profile(sample, batch):
    validate_sample(sample, batch, PROFILE_ITERATIONS, PROFILE_BLOCKS, 'profiled', True)
    pairs = sample.get('clock_pair_median_ns')
    rows = sample.get('phase_samples')
    if (type(pairs) not in (int, float) or not math.isfinite(pairs) or pairs < 0
            or not isinstance(rows, list) or len(rows) != PROFILE_ITERATIONS * PROFILE_BLOCKS):
        raise ValueError('invalid profile samples or clock calibration')
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError('invalid phase row')
        times, visits = row.get('phases'), row.get('visits')
        if any(not isinstance(v, dict) or set(v) != set(PHASES)
               or any(type(n) is not int or n < 0 for n in v.values()) for v in (times, visits)):
            raise ValueError('invalid phase fields')
        if (type(row.get('total_ns')) is not int or row['total_ns'] <= 0
                or sum(times.values()) != row['total_ns']
                or type(row.get('clock_reads')) is not int or row['clock_reads'] < 2):
            raise ValueError('invalid exclusive phase partition')
        required = set(PHASES) - {'entry_residual', 'session_prepare'}
        if (any(visits[p] < 1 for p in required) or visits['registration'] != 3
                or visits['facade_lock'] != 4 or visits['ort_run'] != 1 or visits['oracle'] != 1
                or visits['session_prepare'] != 0 or times['session_prepare'] != 0):
            raise ValueError('profile did not follow the warm compiled path')
    totals = {p: sum(r['phases'][p] for r in rows) for p in PHASES}
    total = sum(r['total_ns'] for r in rows)
    return dict(cell=f'b{batch}-t1', completed_calls=len(rows),
                mean_instrumented_ns_per_call=total / len(rows),
                clock_pair_median_ns=pairs,
                mean_clock_reads_per_call=statistics.mean(r['clock_reads'] for r in rows),
                phases={p: dict(mean_ns_per_call=totals[p] / len(rows),
                                fraction_of_instrumented_total=totals[p] / total) for p in PHASES})


def sample_run(argv, out, stem, batch, iterations, blocks, energy=False, runner='head'):
    sampler, joules, reason, samples = None, None, 'energy_run_not_requested', 0
    if energy:
        try:
            sampler = ComponentEnergySampler()
            sampler.start()
            reason = 'cpu_component_energy_not_process_attributed'
        except (RuntimeError, OSError) as exc:
            sampler = None
            reason = str(exc)
    try:
        elapsed = command(argv, out, stem)
    finally:
        if sampler:
            try:
                joules, samples = sampler.stop()
            except (RuntimeError, OSError) as exc:
                reason, joules = str(exc), None
    sample = json.loads((out / (stem + '.stdout')).read_text())
    validate_sample(sample, batch, iterations, blocks, runner)
    if energy:
        # The process window includes cold preparation, eight warmups, timed
        # blocks, validation and teardown. All calls must pass before counting.
        completed = (1 + 8 + iterations * blocks) * batch
        available = (type(joules) in (int, float) and math.isfinite(joules) and joules > 0
                     and math.isfinite(elapsed) and elapsed > 0)
        if joules is not None and not available:
            reason = 'invalid_component_energy_observation'
        evidence = dict(available=available, evidence_class='E1' if available else None,
                        hardware_measured_joules=joules if available else None,
                        joules_per_verified_vector=joules / completed if available else None,
                        calibrated_joules_estimate=None, analytical_joules_estimate=None,
                        completed_verified_vectors=completed, process_elapsed_seconds=elapsed,
                        average_component_watts=joules / elapsed if available else None,
                        boundary=(sampler.boundary if sampler else 'unavailable'),
                        timing_boundary='whole child process including setup, warmups and teardown',
                        samples=samples, reason=reason, physical_system_energy_measured=False,
                        claim_authorized=False)
        write(out / (stem + '.energy.json'), evidence)
    return sample


def capture(args):
    import numpy as np
    import onnxruntime as ort
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    if any(out.iterdir()):
        raise ValueError('use a fresh output directory')
    build, sdk = args.build.resolve(), args.sdk.resolve()
    base = args.base_build.resolve() if args.base_build else None
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    if args.head_sha and revision != args.head_sha:
        raise ValueError('head revision mismatch')
    manifest = dict(schema='shorthand.generated_infer.bundle.v2', head_sha=revision,
                    base_sha=args.base_sha, compiler_revision=revision,
                    comparison_scope='same generated object, different runtime archives; compiler changes not compared',
                    working_tree_diff_sha256=hashlib.sha256(subprocess.check_output(['git', 'diff', 'HEAD'], cwd=ROOT)).hexdigest(),
                    hardware=cpu_identity(), platform=platform.platform(), boundary=BOUNDARY,
                    workload='synthetic zero-input FP32 MatMul+Add; no classification accuracy claim',
                    functional_unit='one numerically verified FP32 output vector (10 scores)',
                    iterations=args.iterations, blocks=args.blocks, rounds=args.rounds,
                    latency_claim_authorized=False, energy_claim_authorized=False,
                    unsupported_compiled_cells=['b16-t2', 'b16-t4'],
                    unsupported_reason='compiled C ABI currently uses one ORT intra-op thread', observations=[], profiles=[])
    # No launch/cold/setup times are mixed into the resident block ratios.
    runners = ['head', 'base', 'direct', 'direct_all'] if base else ['head', 'direct', 'direct_all']
    manifest['runners'] = runners
    manifest['configurations'] = {runner: configuration(runner) for runner in runners}
    manifest['executables'] = {str(build / 'short_hand'): sha(build / 'short_hand'),
                              str(build / 'libshorthand_runtime.a'): sha(build / 'libshorthand_runtime.a'),
                              str(sdk / 'lib/libonnxruntime.so'): sha(sdk / 'lib/libonnxruntime.so')}
    profiled_library = build / 'libshorthand_runtime_profiled.a'
    manifest['executables'][str(profiled_library)] = sha(profiled_library)
    if base:
        manifest['executables'][str(base / 'libshorthand_runtime.a')] = sha(base / 'libshorthand_runtime.a')
    for batch in (1, 16, 32):
        cell = out / f'b{batch}-t1'
        cell.mkdir()
        create(cell, batch=batch, threads=1)
        model = cell / 'digits.onnx'
        options = ort.SessionOptions()
        options.intra_op_num_threads = options.inter_op_num_threads = 1
        options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_BASIC
        reference = ort.InferenceSession(str(model), options, providers=['CPUExecutionProvider'])
        reference.run(None, {'X': np.zeros((batch, 64), dtype=np.float32)})[0].astype('<f4').tofile(cell / 'expected.bin')
        source = cell / 'inference.short'
        source.write_text(f'''int invocation;
tensor input float "{batch},64";
tensor output float "{batch},10";
model classifier {{
  format onnx;
  path "{model}";
  task "compiled_zero_input_probe";
  precision float;
  input_shape "{batch},64";
  output_shape "{batch},10";
  backend_preference onnxruntime_cpu;
  compact false;
  quality_guardrail accuracy >= 0;
}};
infer classifier(input) -> output;
''')
        command([build / 'short_hand', source, 'compile-mlir', '--output', cell / 'inference.ll'], cell, 'compile-source')
        command([args.clang, '-O2', '-fno-fast-math', '-c', cell / 'inference.ll', '-o', cell / 'inference.o'], cell, 'compile-ir')
        command(['objcopy', '--redefine-sym', 'main=shorthand_entry', cell / 'inference.o'], cell, 'rename-entry')
        binaries = {}
        for runner in [*runners, 'profiled']:
            binary = cell / runner
            argv = [args.clang, '-std=c++17', '-O2', '-fno-fast-math', '-pthread',
                    '-DSHORTHAND_GENERATED=' + str(int(not runner.startswith('direct'))),
                    '-DSHORTHAND_ORT_ALL=' + str(int(runner == 'direct_all')),
                    '-DSHORTHAND_RUNTIME_PHASE_PROFILE=' + str(int(runner == 'profiled')),
                    '-I' + str(ROOT / 'Compiler_new_ws/Short_Hand/src'), '-I' + str(sdk / 'include'),
                    ROOT / 'experiments/energy/generated_infer_probe.cpp']
            if not runner.startswith('direct'):
                archive = profiled_library if runner == 'profiled' else (base if runner == 'base' else build) / 'libshorthand_runtime.a'
                argv += [cell / 'inference.o', archive,
                         '-Wl,--wrap=short_ai_infer_f32']
            if runner == 'profiled':
                # Retain executable code and symbols, without duplicating the
                # large archive's DWARF in every diagnostic artifact binary.
                argv += ['-Wl,--strip-debug']
            argv += ['-L' + str(sdk / 'lib'), '-Wl,-rpath,' + str(sdk / 'lib'), '-lonnxruntime', '-o', binary]
            command(argv, cell, 'link-' + runner)
            binaries[runner] = binary
        frozen = {path: sha(path) for path in [model, source, cell / 'expected.bin', *binaries.values()]}
        sequence = []
        for _ in range(args.rounds):
            sequence += runners + list(reversed(runners))
        for index, runner in enumerate(sequence):
            stem = f'latency-{index:02}-{runner}'
            sample = sample_run([binaries[runner], model, str(batch), cell / 'expected.bin', str(args.iterations), str(args.blocks)],
                                cell, stem, batch, args.iterations, args.blocks, runner=runner)
            manifest['observations'].append(dict(cell=cell.name, runner=runner, sequence=index, path=f'{cell.name}/{stem}.stdout'))
        # Component metering runs are separate from the latency sample series.
        for runner in runners:
            sample_run([binaries[runner], model, str(batch), cell / 'expected.bin', str(args.iterations), str(args.blocks)],
                       cell, 'energy-' + runner, batch, args.iterations, args.blocks, energy=True, runner=runner)
        # No instrumented binary participates in latency ratios or energy runs.
        command([binaries['profiled'], model, str(batch), cell / 'expected.bin',
                 str(PROFILE_ITERATIONS), str(PROFILE_BLOCKS)], cell, 'phases')
        validate_profile(json.loads((cell / 'phases.stdout').read_text()), batch)
        manifest['profiles'].append(dict(cell=cell.name, path=f'{cell.name}/phases.stdout'))
        if any(sha(path) != digest for path, digest in frozen.items()):
            raise ValueError('workload or executable changed during capture')
    if any(sha(pathlib.Path(path)) != digest for path, digest in manifest['executables'].items()):
        raise ValueError('compiler/runtime changed during capture')
    manifest['hashes'] = {str(p.relative_to(out)): sha(p) for p in sorted(out.rglob('*')) if p.is_file()}
    write(out / 'manifest.json', manifest)
    (out / 'manifest.sha256').write_text(sha(out / 'manifest.json') + '\n')
    return replay(out, sha(out / 'manifest.json'))


def replay(out, expected_digest):
    if sha(out / 'manifest.json') != expected_digest:
        raise ValueError('manifest hash mismatch')
    manifest = json.loads((out / 'manifest.json').read_text())
    if not isinstance(manifest, dict) or manifest.get('schema') != 'shorthand.generated_infer.bundle.v2':
        raise ValueError('invalid generated manifest')
    if manifest.get('runners') not in (['head', 'direct', 'direct_all'], ['head', 'base', 'direct', 'direct_all']):
        raise ValueError('invalid generated runners')
    if manifest.get('configurations') != {r: configuration(r) for r in manifest['runners']}:
        raise ValueError('invalid manifest control configurations')
    for name, low, high in [('rounds', 1, 8), ('iterations', 1, 4096), ('blocks', 2, 20)]:
        if type(manifest.get(name)) is not int or not low <= manifest[name] <= high:
            raise ValueError('invalid generated protocol: ' + name)
    if any(manifest.get(k) is not False for k in ('latency_claim_authorized', 'energy_claim_authorized')):
        raise ValueError('generated manifest cannot authorize reduction claims')
    if not isinstance(manifest.get('hashes'), dict) or not manifest['hashes']:
        raise ValueError('missing artifact hashes')
    for name, digest in manifest['hashes'].items():
        relative = pathlib.PurePosixPath(name)
        if (relative.is_absolute() or '..' in relative.parts or str(relative) != name
                or not isinstance(digest, str) or len(digest) != 64
                or any(c not in '0123456789abcdef' for c in digest)):
            raise ValueError('invalid artifact path or digest')
        path = out / name
        if (any(p.is_symlink() for p in [path, *path.parents] if p != out.parent)
                or not path.resolve().is_relative_to(out.resolve()) or not path.is_file() or sha(path) != digest):
            raise ValueError('artifact hash mismatch: ' + name)
    observations = manifest.get('observations')
    if (not isinstance(observations, list) or len(observations) != 3 * len(manifest['runners']) * 2 * manifest['rounds']
            or any(not isinstance(r, dict) or r.get('cell') not in ('b1-t1', 'b16-t1', 'b32-t1')
                   or r.get('runner') not in manifest['runners'] or type(r.get('path')) is not str
                   or r['path'] not in manifest['hashes'] for r in observations)):
        raise ValueError('invalid generated observation matrix')
    if len({r['path'] for r in observations}) != len(observations):
        raise ValueError('duplicate generated observation paths')
    rows = []
    for batch in (1, 16, 32):
        groups = {}
        for runner in manifest['runners']:
            matches = [r for r in manifest['observations'] if r['cell'] == f'b{batch}-t1' and r['runner'] == runner]
            if len(matches) != 2 * manifest['rounds'] or len({r['path'] for r in matches}) != len(matches):
                raise ValueError('missing or duplicate generated observations')
            values = []
            for record in matches:
                if record['path'] not in manifest['hashes']:
                    raise ValueError('unhashed sample')
                sample = json.loads((out / record['path']).read_text())
                validate_sample(sample, batch, manifest['iterations'], manifest['blocks'], runner)
                values.extend(t / (batch * sample['iterations']) for t in sample['block_elapsed_ms'])
            groups[runner] = dict(mean_ms_per_vector=statistics.mean(values),
                                  median_block_ms_per_vector=statistics.median(values),
                                  minimum_block_ms_per_vector=min(values), maximum_block_ms_per_vector=max(values),
                                  sample_blocks=len(values))
        rows.append(dict(cell=f'b{batch}-t1', observations=groups,
                         head_base_ratio=groups['head']['mean_ms_per_vector'] / groups['base']['mean_ms_per_vector'] if 'base' in groups else None,
                         head_direct_ratio=groups['head']['mean_ms_per_vector'] / groups['direct']['mean_ms_per_vector'],
                         head_direct_all_ratio=groups['head']['mean_ms_per_vector'] / groups['direct_all']['mean_ms_per_vector'],
                         direct_all_basic_ratio=groups['direct_all']['mean_ms_per_vector'] / groups['direct']['mean_ms_per_vector']))
    profiles = manifest.get('profiles')
    if (not isinstance(profiles, list) or len(profiles) != 3
            or any(not isinstance(p, dict) or not isinstance(p.get('path'), str)
                   or p['path'] not in manifest['hashes'] for p in profiles)
            or {p.get('cell') for p in profiles} != {'b1-t1', 'b16-t1', 'b32-t1'}
            or len({p['path'] for p in profiles}) != 3
            or {p['path'] for p in profiles} & {r['path'] for r in observations}):
        raise ValueError('invalid separate profiling matrix')
    phase_rows = [validate_profile(json.loads((out / p['path']).read_text()), int(p['cell'][1:-3])) for p in profiles]
    phase_report = dict(schema='shorthand.generated_infer.phases.v1', success=True, manifest_sha256=expected_digest,
                        rows=phase_rows, instrumented=True, latency_claim_authorized=False, energy_claim_authorized=False,
                        boundary='exclusive phases of one warm compiled entry including independent output verification; clock overhead included')
    write(out / 'phases.json', phase_report)
    report = dict(schema='shorthand.generated_infer.report.v2', success=True,
                  manifest_sha256=expected_digest, rows=rows, boundary=BOUNDARY,
                  latency_claim_authorized=False, energy_claim_authorized=False)
    write(out / 'report.json', report)
    lines = ['# Compiled FP32 observations', '', manifest['workload'], '', BOUNDARY, '',
             'Descriptive shared-runner observations; no latency or energy claim is authorized.', '',
             'Both direct controls use one intra/inter thread, sequential execution, spinning disabled, and preallocated input/output. ALL is an explicit tuning candidate, not a claim of globally optimal tuning.', '',
             '| Cell | Head ms/vector | Head/base | Head/ORT BASIC | Head/ORT ALL | ORT ALL/BASIC |', '|---|---:|---:|---:|---:|---:|']
    for row in rows:
        ratio = f"{row['head_base_ratio']:.4f}" if row['head_base_ratio'] is not None else 'unavailable'
        lines.append(f"| {row['cell']} | {row['observations']['head']['mean_ms_per_vector']:.6f} | {ratio} | {row['head_direct_ratio']:.4f} | {row['head_direct_all_ratio']:.4f} | {row['direct_all_basic_ratio']:.4f} |")
    lines += ['', '## Diagnostic phase profile', '',
              'Separate instrumented binary, 64 verified warm calls/cell. Exclusive phases sum to the instrumented total. Clock overhead is included, never subtracted. These are attribution observations, not latency or energy controls.', '',
              '| Phase | b1 mean ns/call | b16 mean ns/call | b32 mean ns/call |', '|---|---:|---:|---:|']
    for phase in PHASES:
        by_cell = {r['cell']: r for r in phase_rows}
        lines.append('| ' + phase + ' | ' + ' | '.join(f"{by_cell[f'b{b}-t1']['phases'][phase]['mean_ns_per_call']:.1f}" for b in (1, 16, 32)) + ' |')
    lines += ['', 'Energy is recorded separately in each `energy-*.energy.json`; unavailable counters produce null joules.',
              'b16-t2 and b16-t4 are unsupported for compiled hooks; all five real-digit cells remain in resident evidence.',
              'These are zero-input inference vectors, not held-out digit accuracy or end-to-end serving measurements.', '']
    (out / 'GENERATED.md').write_text('\n'.join(lines))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--build', type=pathlib.Path)
    parser.add_argument('--sdk', type=pathlib.Path)
    parser.add_argument('--base-build', type=pathlib.Path)
    parser.add_argument('--head-sha')
    parser.add_argument('--base-sha')
    parser.add_argument('--clang', default='clang++-18')
    parser.add_argument('--iterations', type=int, default=256)
    parser.add_argument('--blocks', type=int, default=4)
    parser.add_argument('--rounds', type=int, default=2)
    parser.add_argument('--replay-sha256')
    args = parser.parse_args()
    if args.replay_sha256:
        replay(args.output, args.replay_sha256)
    else:
        if not args.build or not args.sdk or not 1 <= args.iterations <= 4096 or not 2 <= args.blocks <= 20 or not 1 <= args.rounds <= 8:
            parser.error('build/sdk and bounded positive protocol required')
        capture(args)


if __name__ == '__main__':
    main()
