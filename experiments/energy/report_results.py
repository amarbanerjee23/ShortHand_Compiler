#!/usr/bin/env python3
"""Export verified summaries, every paired timing, and a readable Markdown report."""
import argparse
import hashlib
import json
import pathlib
import shutil


def read(path):
    return json.loads(path.read_text())


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def trial_windows(trials):
    # These are the independent process/inner-trial observations used in the
    # analysis. Per-batch diagnostic arrays remain in the full capture artifact.
    keys = ('start_unix_seconds', 'end_unix_seconds', 'elapsed_ms', 'completed', 'success', 'accuracy')
    return [{key: trial[key] for key in keys if key in trial} for trial in trials]


def export(root, out):
    out.mkdir(parents=True, exist_ok=False)
    if not (root / 'run.json').is_file():
        (out / 'RESULTS.md').write_text('# Capture failed before experiments started\n\nSee workflow logs. No performance or energy result is available.\n')
        return
    run = read(root / 'run.json')
    energy_status = read(root / 'energy-status.json') if (root / 'energy-status.json').is_file() else {
        'schema': 'shorthand.energy.availability.v1',
        'physical_energy_measured': False,
        'energy_savings_percent': None,
        'source': 'missing',
    }
    passed = {stage['name'] for stage in run['stages'] if stage['success']}
    records = dict(schema='shorthand.energy.results-export.v1', run=run, cells=[],
                   source_manifests={}, energy_savings_percent=None,
                   energy_status=energy_status,
                   exporter_sha256=digest(pathlib.Path(__file__)))
    observations = []
    for name in ('source-r10', 'source-r100'):
        directory = root / name
        if name not in passed or not (directory / 'sota-summary.json').is_file():
            continue
        summary = read(directory / 'sota-summary.json')
        capture = read(directory / 'capture.json')
        plan = read(directory / 'plan.json')
        records['source_manifests'][name] = digest(directory / 'manifest.json')
        for cell in summary['cells']:
            records['cells'].append(dict(track='source', campaign=name, baseline=cell['baseline'],
                repetitions=plan['repetitions'], accuracy=capture['expected']['accuracy'],
                metric=cell['metric'], comparison=cell['comparison']))
        for pair in capture['pairs']:
            observations.append(dict(campaign=name, baseline=pair['baseline_name'],
                pair=pair['index'], order=pair['order'], shorthand=pair['shorthand'], peer=pair['peer']))
    directory = root / 'python-runtime'
    if 'python-runtime' in passed and (directory / 'summary.json').is_file():
        summary = read(directory / 'summary.json')
        records['source_manifests']['python-runtime'] = digest(directory / 'manifest.json')
        for cell in summary['runtime_cells']:
            records['cells'].append(dict(track='runtime', campaign='python-runtime',
                baseline='Python / ONNX', cell=cell['id'], metric=cell['metric'], comparison=cell['comparison']))
            base = directory / cell['id']
            comparison = read(base / 'comparison.json')
            for n, pair in enumerate(comparison['pairs']):
                a, b = (read(base / pair[k + '_file']) for k in ('native', 'python'))
                observations.append(dict(campaign='python-runtime', cell=cell['id'], pair=n,
                    order=pair['order'], shorthand=trial_windows(a['trials']), peer=trial_windows(b['trials']), accuracy=a['accuracy'],
                    predictions_sha256=hashlib.sha256(json.dumps(a['predictions']).encode()).hexdigest()))
    directory = root / 'cpp-runtime'
    if 'cpp-runtime' in passed and (directory / 'manifest.json').is_file():
        summary = read(directory / 'summary.json')
        records['source_manifests']['cpp-runtime'] = digest(directory / 'manifest.json')
        for cell in summary['cells']:
            records['cells'].append(dict(track='runtime', campaign='cpp-runtime', baseline='C++ / ONNX',
                cell=cell['id'], metric=cell['metric'], comparison=cell['comparison']))
            report = read(directory / 'cells' / cell['id'] / 'report.json')
            for n, pair in enumerate(report['pairs']):
                observations.append(dict(campaign='cpp-runtime', cell=cell['id'], pair=n,
                    order=pair['order'], shorthand=trial_windows(pair['native_trials']), peer=trial_windows(pair['cpp_trials'])))
    write(out / 'summary.json', records)
    write(out / 'observations.json', observations)
    # Keep environment, frozen plans, raw source timings and build costs in Git.
    # The complete workflow artifact also contains binaries, outputs and scores.
    metadata = out / 'metadata'
    for pattern in ('run.json', 'pip-freeze.txt', 'pip-torch-install.json', 'lscpu.json', 'energy-status.json', 'energy-probe.json', 'energy-probe.txt',
                    '*-failure.txt', '*-plan/plan.json', '*/environment.json', '*/code.json',
                    '*/capture.json', '*/manifest.json', 'python-runtime/source/source.json'):
        for path in sorted(root.glob(pattern)):
            target = metadata / path.relative_to(root)
            if path.name == 'energy-probe.json':
                target = target.with_suffix('.txt')  # The native probe emits plain text.
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
    url = 'https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/' + str(run['run_id'])
    lines = ['# Executed comparison results', '',
        '**Energy savings: not measured.** This capture records elapsed time and correctness. '
        'There is no calibrated whole-host power trace, so joules, energy savings, carbon savings '
        'and energy break-even are unavailable. Latency reductions below are not energy reductions.', '',
        f'Capture started: {run["started_utc"]}. [Workflow and full evidence artifact]({url}).',
        f'Tested revision: `{run["revision"]}`. Platform: `{run["platform"]}`. '
        f'Available CPU affinity: `{run["affinity"]}`.', '',
        '## Completion and coverage', '', '| Stage | Status |', '| --- | --- |']
    for stage in run['stages']:
        lines.append(f'| {stage["name"]} | {"PASS" if stage["success"] else "FAILED; retained in metadata"} |')
    lines += ['', 'Rust/Candle and Mojo/MAX were **not executed or implementation-verified**: '
        'the repository supplies external runner contracts but no runners. The `torch` profile '
        'covers the four implemented source baselines and does not qualify the `full` matrix.', '',
        '## Source workload: FP64 nearest-centroid classification', '',
        'UCI Optdigits: 3,823 training rows and 1,797 held-out test rows; train-only centroids, '
        '64 features and 10 classes. Every invocation must match all 1,797 reference predictions '
        'and the repetition checksum. Shorthand uses LLVM/Clang 18 `-O2`; C++17 uses '
        '`-O3`; both disable fast-math. NumPy/PyTorch use one CPU thread.', '',
        'Each row has 30 balanced randomized process pairs after two warmups. Timing includes '
        'process startup, Python/framework imports, model loading where applicable, input parsing, '
        'normalization, repeated classification, output and teardown. Shorthand/C++ embed weights '
        'in their binaries; Python loads JSON. TorchInductor reuses its warmed disk cache but '
        '**every measured process still performs startup and tracing**. This is not resident '
        'PyTorch inference or a pure kernel/language comparison.', '',
        '| Repetitions | Baseline | Shorthand ms/image | Baseline ms/image | Latency reduction % | Paired 95% interval % | Accuracy % |',
        '| ---: | --- | ---: | ---: | ---: | --- | ---: |']
    for cell in records['cells']:
        if cell['track'] != 'source':
            continue
        c = cell['comparison']
        lo, hi = c['paired_bootstrap_95_percent_interval']
        lines.append(f'| {cell["repetitions"]} | {cell["baseline"]} | {c["native_mean"]:.6f} | '
                     f'{c["python_mean"]:.6f} | {c["savings_percent"]:.2f} | [{lo:.2f}, {hi:.2f}] | {100 * cell["accuracy"]:.3f} |')
    lines += ['', '## Runtime workload: FP32 ONNX Runtime 1.30.0 CPU', '',
        'The same FP32 ONNX model and raw dataset are used by Shorthand AIRuntime, Python/NumPy '
        'ONNX Runtime and the independent C++17 ONNX Runtime executable. This measures host '
        'integration overhead around the same inference backend, separately from the FP64 source experiment.', '',
        'Ten balanced process pairs per cell; each process contains three resident-session trials, '
        'each classifying 1,797 images 10 times. One process mean is the bootstrap unit, so inner '
        'trials are not treated as independent samples. Session preparation, warmup and final '
        'serialization are outside the trial window; validation, normalization, inference and '
        'top-k processing are inside. Batch size varies at one thread; thread count varies at batch 16.', '',
        '| Cell (batch / threads) | Baseline | Shorthand ms/image | Baseline ms/image | Latency reduction % | Paired 95% interval % |',
        '| --- | --- | ---: | ---: | ---: | --- |']
    for cell in records['cells']:
        if cell['track'] != 'runtime':
            continue
        c = cell['comparison']
        lo, hi = c['paired_bootstrap_95_percent_interval']
        lines.append(f'| {cell["cell"]} | {cell["baseline"]} | {c["native_mean"]:.6f} | '
                     f'{c["python_mean"]:.6f} | {c["savings_percent"]:.2f} | [{lo:.2f}, {hi:.2f}] |')
    lines += ['', '## What the measurements show', '']
    source = {(c['repetitions'], c['baseline']): c['comparison'] for c in records['cells'] if c['track'] == 'source'}
    if all(k in source for k in ((10, 'numpy'), (10, 'cpp17-o3'), (100, 'numpy'), (100, 'cpp17-o3'))):
        lines += [f'At ten repetitions, source-process latency was {source[10, "numpy"]["savings_percent"]:.2f}% lower than NumPy '
            f'and {source[10, "cpp17-o3"]["savings_percent"]:.2f}% lower than C++. At 100 repetitions, these reductions were '
            f'{source[100, "numpy"]["savings_percent"]:.2f}% and {source[100, "cpp17-o3"]["savings_percent"]:.2f}%. '
            'The smaller advantage with more work per process shows why startup and workload size matter.', '']
    cpp = [c['comparison']['native_over_python_ratio'] for c in records['cells'] if c.get('campaign') == 'cpp-runtime']
    if cpp and all(r > 1 for r in cpp):
        lines += [f'**Shorthand AIRuntime was slower than independent C++/ONNX in every cell: {min(cpp):.2f}–{max(cpp):.2f}× '
            'the latency.** The Python comparison also changes with batching: read every cell rather than selecting '
            'the batch-1 improvement. These results do not support a general runtime-efficiency advantage.', '']
    lines += ['Large fresh-process reductions versus PyTorch include framework startup and tracing. '
        'They do not establish an advantage over resident PyTorch inference. Runtime gaps require profiling '
        'before their causes can be assigned to validation, memory handling, instrumentation or backend integration.', '',
        '## Interpretation and evidence', '',
        'Reduction = `100 × (1 − mean(Shorthand) / mean(baseline))`. Negative values mean '
        'Shorthand was slower. All pairs are retained; no outlier filtering or best-run selection '
        'is used. Intervals use 10,000 paired-block bootstrap resamples, seed 104. They are '
        'exploratory within-session intervals without multiple-comparison correction.', '',
        'This is one shared hosted-runner session with one small classifier. Startup costs, '
        'virtualization, CPU scheduling, batching and implementation choices affect the numbers. '
        'The 10/100 repetition comparison changes startup amortization; it does not isolate it. '
        'These results cannot establish a universal language, framework or energy ranking.', '',
        '- [summary.json](summary.json): exact numbers and all per-pair values used by the bootstrap.',
        '- [observations.json](observations.json): ordered pairs, raw durations, completed work and inner trial windows.',
        '- [metadata/](metadata/): frozen plans, source captures, build timings, environment, energy availability and original bundle digests.',
        '- Full artifact: raw stdout/stderr, predictions/scores, binaries, LLVM IR and replay manifests.', '',
        'Reproduce with the `experiment-results` workflow, or follow '
        '[the experiment instructions](../../README.md). Use `run_verified.py` with the same '
        'pinned dependencies and built compiler. Real energy work requires the calibrated-energy '
        'protocol, an independently logging whole-host meter, and repeated dedicated-machine sessions.']
    (out / 'RESULTS.md').write_text('\n'.join(lines) + '\n')
    write(out / 'SHA256SUMS.json', {str(p.relative_to(out)): digest(p) for p in sorted(out.rglob('*')) if p.is_file()})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=pathlib.Path, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    export(args.input, args.output)
