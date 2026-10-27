#!/usr/bin/env python3
"""Same-host immutable/mutable observations and separate validated model-I/O traces."""
import argparse
import hashlib
import json
import math
import os
import pathlib
import re
import shutil
import statistics
import subprocess
import sys
import time

from ci_energy_evidence import ComponentEnergySampler, cpu_identity

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/ai_application'))
from test_immutable_model import compile_probe, fixtures

CELLS = ('identity', 'benchmark-64k', 'benchmark-1m')
MODES = ('immutable', 'mutable')
BLOCKS = 6
ITERATIONS = 2048
TRACE_ITERATIONS = 16


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def sample(text, mode, iterations):
    lines = text.splitlines()
    require(len(lines) == 3 and lines[:2] == ['MODEL_WARM_BEGIN', 'MODEL_WARM_END'], 'missing native warm boundaries')
    value = json.loads(lines[2])
    require(value.get('mode') == mode and type(value.get('iterations')) is int and value['iterations'] == iterations
            and type(value.get('verified_calls')) is int and value['verified_calls'] == iterations + 9, 'wrong native workload')
    require(all(type(value.get(k)) is int and value[k] > 0 for k in ('warm_ns', 'cold_ns')), 'invalid native timing')
    require(value.get('mutable_preparations') == (1 if mode == 'mutable' else 0), 'control preparation count')
    return value


def trace_counts(text):
    lines = text.splitlines()
    starts = [i for i, line in enumerate(lines) if 'write(1' in line and '"MODEL_WARM_BEGIN\\n"' in line]
    ends = [i for i, line in enumerate(lines) if 'write(1' in line and '"MODEL_WARM_END\\n"' in line]
    require(len(starts) == len(ends) == 1 and starts[0] < ends[0], 'invalid syscall trace boundaries')
    require('+++ exited with 0 +++' in text and '+++ killed by' not in text, 'incomplete native syscall trace')
    warm = lines[starts[0] + 1:ends[0]]
    # -f includes worker threads; -yy resolves descriptor paths on reads/maps.
    # All ONNX paths count, including failed opens. Startup/digest sidecars are
    # outside the declared warm window and do not enter these counts.
    hits = [line for line in warm if re.search(r'\.onnx(?:[">]|\\")', line)
            and re.search(r'\b(?:open|openat|openat2|read|pread64|mmap|stat|statx|newfstatat|access)\(', line)]
    opens = sum(bool(re.search(r'\b(?:open|openat|openat2)\(', line)) for line in hits)
    reads = sum(bool(re.search(r'\b(?:read|pread64)\(', line)) for line in hits)
    return dict(model_io_syscalls=len(hits), model_opens=opens, model_reads=reads)


def execute(out, name, argv, allow_failure=False):
    argv = list(map(str, argv))
    write(out / (name + '.command.json'), argv)
    start = time.perf_counter()
    with (out / (name + '.stdout')).open('w') as stdout, (out / (name + '.stderr')).open('w') as stderr:
        result = subprocess.run(argv, stdout=stdout, stderr=stderr, check=False, timeout=240)
    write(out / (name + '.exit.json'), dict(returncode=result.returncode, elapsed_seconds=time.perf_counter() - start))
    require(allow_failure or result.returncode == 0, name + ' failed; see retained stderr')
    return result.returncode


def meter(out, name, argv):
    sampler, active, joules, samples, reason = None, False, None, 0, 'unavailable'
    try:
        sampler = ComponentEnergySampler()
        sampler.start()
        active = True
    except Exception as error:
        reason = str(error)
    try:
        execute(out, name, argv)
    finally:
        if active:
            try:
                joules, samples = sampler.stop()
                reason = 'component_energy_not_process_attributed'
            except Exception as error:
                reason, joules = str(error), None
    available = type(joules) in (int, float) and math.isfinite(joules) and joules > 0 and samples >= 2
    write(out / (name + '.energy.json'), dict(available=available, evidence_class='E1' if available else None,
          hardware_measured_joules=joules if available else None, samples=samples,
          joules_per_verified_call=joules / (ITERATIONS + 9) if available else None,
          verified_calls=ITERATIONS + 9, method=sampler.method if sampler else None,
          reason=reason, physical_system_energy_measured=False, claim_authorized=False,
          boundary='whole child process, including setup/first call/8 warmups/timed calls/teardown; CPU component only'))


def capture(args):
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    fixtures(out / 'fixtures')
    binary = out / 'probe'
    compile_command = compile_probe(args.build.resolve(), args.sdk.resolve(), binary)
    write(out / 'compile.command.json', compile_command)
    sdk = args.sdk.resolve() / 'lib/libonnxruntime.so'
    shutil.copy2(sdk.resolve(), out / 'libonnxruntime.snapshot')
    manifest = dict(schema='shorthand.immutable_model.bundle.v1',
                    revision=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                    hardware=cpu_identity(), cells=list(CELLS), iterations=ITERATIONS, blocks=BLOCKS,
                    compiler=subprocess.check_output([compile_command[0], '--version'], text=True),
                    source_diff='source.diff', observations=[], energy=[], traces=[],
                    latency_claim_authorized=False, energy_claim_authorized=False)
    (out / 'source.diff').write_bytes(subprocess.check_output(['git', 'diff', 'HEAD'], cwd=ROOT))
    source_files = ['tests/ai_application/test_immutable_model.cpp', 'tests/ai_application/test_immutable_model.py',
                    'experiments/energy/immutable_model_evidence.py', 'experiments/energy/ci_energy_evidence.py',
                    'Compiler_new_ws/Short_Hand/src/runtime/ImmutableModelInternal.h',
                    'Compiler_new_ws/Short_Hand/src/runtime/ImmutableOnnxEnvelope.h',
                    'Compiler_new_ws/Short_Hand/src/runtime/RuntimeThreadSafeFacade.cpp',
                    'Compiler_new_ws/Short_Hand/src/ai_runtime/AI_Runtime.cpp']
    for source in source_files:
        path = out / 'source' / source
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / source, path)
    manifest['sources'] = ['source/' + source for source in source_files]
    for cell in CELLS:
        model_path = out / 'fixtures' / (cell + '.onnx')
        for block in range(BLOCKS):
            for mode in MODES if block % 2 == 0 else MODES[::-1]:
                name = f'{cell}-{block}-{mode}'
                execute(out, name, [binary, mode, model_path, ITERATIONS])
                manifest['observations'].append(dict(cell=cell, block=block, mode=mode, name=name))
        for mode in MODES:
            name = f'{cell}-energy-{mode}'
            meter(out, name, [binary, mode, model_path, ITERATIONS])
            manifest['energy'].append(dict(cell=cell, mode=mode, name=name))
    for mode in (*MODES, 'injected-read'):
        name = 'trace-' + mode
        if shutil.which('strace') is None:
            raise ValueError('strace must be installed to retain a trace attempt')
        result = execute(out, name, ['strace', '-f', '-yy', '-s', '4096', '-e',
                         'trace=%file,read,pread64,mmap,write', '-o', out / (name + '.strace'),
                         binary, mode, out / 'fixtures/benchmark-1m.onnx', TRACE_ITERATIONS],
                         allow_failure=args.allow_unavailable_trace)
        manifest['traces'].append(dict(mode=mode, name=name, available=result == 0))
    manifest['hashes'] = {str(p.relative_to(out)): digest(p) for p in sorted(out.rglob('*')) if p.is_file()}
    write(out / 'manifest.json', manifest)
    (out / 'manifest.sha256').write_text(digest(out / 'manifest.json') + '\n')
    return replay(out, digest(out / 'manifest.json'))


def replay(out, expected_digest):
    require(digest(out / 'manifest.json') == expected_digest, 'manifest digest mismatch')
    manifest = json.loads((out / 'manifest.json').read_text())
    require(manifest.get('schema') == 'shorthand.immutable_model.bundle.v1' and manifest.get('cells') == list(CELLS)
            and manifest.get('iterations') == ITERATIONS and manifest.get('blocks') == BLOCKS
            and manifest.get('latency_claim_authorized') is False and manifest.get('energy_claim_authorized') is False, 'invalid manifest')
    hashes = manifest['hashes']
    def retained(name):
        p = pathlib.PurePosixPath(name)
        require(not p.is_absolute() and '..' not in p.parts and name in hashes, 'unhashed/unsafe artifact')
        require(not any((out / pathlib.Path(*p.parts[:i])).is_symlink() for i in range(1, len(p.parts) + 1)), 'symlink artifact')
        require(digest(out / name) == hashes[name], 'artifact digest mismatch: ' + name)
        return out / name
    for name in hashes:
        retained(name)
    for name in ['probe', 'libonnxruntime.snapshot', 'compile.command.json', 'source.diff', *manifest['sources']]:
        retained(name)
    for cell in CELLS:
        p = retained('fixtures/' + cell + '.onnx')
        require(digest(p) == retained('fixtures/' + cell + '.onnx.sha256').read_text(), 'model identity mismatch')
    def validate_run(name, mode, cell, iterations, traced=False):
        command = json.loads(retained(name + '.command.json').read_text())
        exit_record = json.loads(retained(name + '.exit.json').read_text())
        retained(name + '.stderr')
        require(exit_record['returncode'] == 0 and exit_record['elapsed_seconds'] > 0, 'failed native run')
        require(command[-3:] == [mode, command[-2], str(iterations)] and pathlib.Path(command[-2]).name == cell + '.onnx'
                and pathlib.Path(command[-4]).name == 'probe', 'command/workload mismatch')
        require(traced == (pathlib.Path(command[0]).name == 'strace'), 'traced timing mixed with uninstrumented run')
        require(len(command) == (13 if traced else 4), 'unexpected native command arguments')
        if traced:
            require(command[:8] == ['strace', '-f', '-yy', '-s', '4096', '-e',
                                    'trace=%file,read,pread64,mmap,write', '-o']
                    and pathlib.Path(command[8]).name == name + '.strace', 'incomplete syscall tracing flags')
        return sample(retained(name + '.stdout').read_text(), mode, iterations)
    expected = [dict(cell=c, block=b, mode=m, name=f'{c}-{b}-{m}') for c in CELLS for b in range(BLOCKS)
                for m in (MODES if b % 2 == 0 else MODES[::-1])]
    require(manifest['observations'] == expected, 'incomplete or misordered paired matrix')
    values = {(r['cell'], r['block'], r['mode']): validate_run(r['name'], r['mode'], r['cell'], ITERATIONS) for r in expected}
    expected_energy = [dict(cell=c, mode=m, name=f'{c}-energy-{m}') for c in CELLS for m in MODES]
    require(manifest['energy'] == expected_energy, 'incomplete energy matrix')
    energy = []
    for row in expected_energy:
        validate_run(row['name'], row['mode'], row['cell'], ITERATIONS)
        e = json.loads(retained(row['name'] + '.energy.json').read_text())
        require(type(e.get('available')) is bool and e.get('claim_authorized') is False and
                e.get('physical_system_energy_measured') is False and e.get('verified_calls') == ITERATIONS + 9, 'invalid energy status')
        j = e.get('hardware_measured_joules')
        if e['available']:
            require(type(j) in (int, float) and math.isfinite(j) and j > 0 and e['samples'] >= 2 and
                    e['evidence_class'] == 'E1' and e['joules_per_verified_call'] == j / (ITERATIONS + 9), 'invalid metered energy')
        else:
            require(j is None and e['joules_per_verified_call'] is None and e['evidence_class'] is None and e['reason'], 'unavailable energy must be null')
        energy.append(dict(**row, evidence=e))
    traces = manifest['traces']
    require(len(traces) == 3 and [r['mode'] for r in traces] == [*MODES, 'injected-read'], 'missing trace controls')
    trace_results = []
    for row in traces:
        mode, name = row['mode'], row['name']
        require(name == 'trace-' + mode and type(row['available']) is bool, 'invalid trace identity')
        raw = retained(name + '.strace').read_text()
        status = json.loads(retained(name + '.exit.json').read_text())
        retained(name + '.command.json'); retained(name + '.stdout')
        error = retained(name + '.stderr').read_text()
        if row['available']:
            validate_run(name, mode, 'benchmark-1m', TRACE_ITERATIONS, traced=True)
            counts = trace_counts(raw)
            require(counts['model_io_syscalls'] == 0 if mode == 'immutable' else
                    counts['model_opens'] >= TRACE_ITERATIONS and counts['model_reads'] >= TRACE_ITERATIONS,
                    'model-I/O trace contract or positive control failed')
            trace_results.append(dict(**row, **counts))
        else:
            require(status['returncode'] != 0 and ('Operation not permitted' in error or 'Permission denied' in error), 'unqualified trace failure')
            trace_results.append(dict(**row, reason=error.strip()))
    rows = []
    for cell in CELLS:
        ratios = [values[cell, b, 'immutable']['warm_ns'] / values[cell, b, 'mutable']['warm_ns'] for b in range(BLOCKS)]
        rows.append(dict(cell=cell, model_bytes=(out / 'fixtures' / (cell + '.onnx')).stat().st_size,
                    immutable_ns=statistics.median(values[cell, b, 'immutable']['warm_ns'] / ITERATIONS for b in range(BLOCKS)),
                    mutable_ns=statistics.median(values[cell, b, 'mutable']['warm_ns'] / ITERATIONS for b in range(BLOCKS)),
                    paired_ratio_median=statistics.median(ratios), paired_ratio_min=min(ratios), paired_ratio_max=max(ratios)))
    report = dict(schema='shorthand.immutable_model.report.v1', success=True, revision=manifest['revision'], rows=rows,
                  model_io_trace_verified=all(r['available'] for r in traces), traces=trace_results, energy=energy,
                  latency_claim_authorized=False, energy_claim_authorized=False)
    # Derived reports appear only after every native record and raw hash passes.
    write(out / 'report.json', report)
    lines = ['# Immutable model pilot', '', f"Revision: `{manifest['revision']}`; manifest SHA-256: `{expected_digest}`.", '',
             'Identity FP32 [1,4], nonzero checked inputs; six balanced pairs, 8 warmups and 2048 timed calls per child. '
             'Compare internal immutable ownership with AIRuntime mutable-path caching through the same native host. '
             'Both validate inputs, check outputs and commit via private scratch. Immutable also includes the facade lock; '
             'this is not a source-language or C++ superiority comparison.', '',
             '| Model bytes | Immutable ns/call | Mutable ns/call | Paired ratio median (min–max) |', '|---:|---:|---:|---:|']
    for r in rows:
        lines.append(f"| {r['model_bytes']} | {r['immutable_ns']:.1f} | {r['mutable_ns']:.1f} | {r['paired_ratio_median']:.4f} ({r['paired_ratio_min']:.4f}–{r['paired_ratio_max']:.4f}) |")
    lines += ['', f"Warm model-I/O trace verified: **{report['model_io_trace_verified']}**. Separate traces include all threads, "
              'the mutable positive control and an injected-read control. Instrumented durations never enter the latency rows.', '',
              f"Energy available: {sum(e['evidence']['available'] for e in energy)}/6. Separate whole-child-process component "
              'energy includes setup, all 2057 verified calls and teardown. Missing energy is null, not zero. '
              'No calibrated estimate, phase attribution or energy saving is inferred.', '',
              'Cold setup/first-call durations, all observations, commands, source snapshots, raw traces and hashes are retained. '
              'Shared-host short-run ratios are descriptive, without confidence intervals or general reduction claims.', '']
    (out / 'IMMUTABLE.md').write_text('\n'.join(lines))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=pathlib.Path)
    parser.add_argument('--sdk', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--replay-sha256')
    parser.add_argument('--allow-unavailable-trace', action='store_true', help='local diagnostic only; forbidden by the hosted evidence gate')
    args = parser.parse_args()
    result = replay(args.output.resolve(), args.replay_sha256) if args.replay_sha256 else capture(args)
    print('PASS immutable evidence replay; trace verified=' + str(result['model_io_trace_verified']))


if __name__ == '__main__':
    main()
