#!/usr/bin/env python3
"""Equivalent ShortHand native-host / optimized NumPy+ONNX CPU comparison.

Uses identical artifacts, resident raw FP32 inputs, normalization, batch/tail
policy, session options, numerical tolerance, top-k and completed-image units.
Does not measure a source-language compiler speedup or claim carbon superiority.
"""
import argparse
import hashlib
import json
import math
import os
import pathlib
import statistics
import subprocess
import sys
import time

os.environ['ORT_DISABLE_TELEMETRY'] = '1'


def load(path):
    def pairs(items):
        result = {}
        for k, v in items:
            if k in result:
                raise ValueError('duplicate JSON key')
            result[k] = v
        return result
    path = pathlib.Path(path)
    if path.is_symlink() or not path.is_file() or path.stat().st_size > 32 * 1024 * 1024:
        raise ValueError('unsafe or oversized evidence file')
    with path.open('rb') as stream:
        data = stream.read(32 * 1024 * 1024 + 1)
    if len(data) > 32 * 1024 * 1024:
        raise ValueError('evidence file grew beyond its bound')
    return json.loads(data.decode('utf-8'), object_pairs_hook=pairs,
                      parse_constant=lambda _: (_ for _ in ()).throw(ValueError('nonfinite JSON')))


def write(path, value):
    pathlib.Path(path).write_text(json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n')


def sha(path):
    digest = hashlib.sha256()
    with pathlib.Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def execute(args):
    result = subprocess.run(list(map(str, args)), capture_output=True, text=True, timeout=1800,
                            env=dict(os.environ, ORT_DISABLE_TELEMETRY='1'))
    if result.returncode < 0 or any(s in result.stderr for s in ('AddressSanitizer', 'LeakSanitizer', 'runtime error:')):
        raise AssertionError(f'runner crash or sanitizer finding: {result.stderr[-4000:]}')
    if result.returncode:
        raise RuntimeError(f'runner failed ({result.returncode}): {result.stderr[-4000:]}')


def worker(config_path, output, tool):
    import numpy as np
    import onnxruntime as ort
    if ort.__version__ != '1.30.0':
        raise ValueError('equivalent baseline requires pinned ONNX Runtime 1.30.0')
    c = load(config_path)
    q = load(c['qualification_config'])
    metadata = pathlib.Path(output).with_suffix('.metadata.json')
    execute([tool, 'application-describe', config_path, metadata])
    report = load(metadata)
    if sha(c['dataset_path']) != c['dataset_sha256']:
        raise ValueError('Python dataset digest mismatch')
    data = np.loadtxt(c['dataset_path'], delimiter=',', dtype=np.float64, ndmin=2)
    if data.shape[1] != c['features'] + 1 or not np.isfinite(data).all():
        raise ValueError('Python invalid dataset shape or values')
    labels = data[:, -1]
    if np.any(labels != labels.astype(np.int64)) or set(labels) != set(range(c['classes'])):
        raise ValueError('Python invalid labels')
    raw = data[:, :-1].astype(np.float32)
    if np.any(raw < c['input_min']) or np.any(raw > c['input_max']):
        raise ValueError('Python input outside range')
    labels = labels.astype(np.int64)
    options = ort.SessionOptions()
    options.intra_op_num_threads = c['threads']
    options.inter_op_num_threads = 1
    options.execution_mode = ort.ExecutionMode.ORT_SEQUENTIAL
    options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_BASIC
    options.add_session_config_entry('session.intra_op.allow_spinning', '0')
    options.add_session_config_entry('session.inter_op.allow_spinning', '0')
    start = time.perf_counter()
    session = ort.InferenceSession(q['model_path'], sess_options=options, providers=['CPUExecutionProvider'])
    session.disable_fallback()
    prep_ms = (time.perf_counter() - start) * 1000
    if len(session.get_inputs()) != 1 or len(session.get_outputs()) != 1 or session.get_providers() != ['CPUExecutionProvider']:
        raise ValueError('Python requires exactly one CPU input and output')
    name, output_name = session.get_inputs()[0].name, session.get_outputs()[0].name
    batch, classes, features = q['batch_size'], c['classes'], c['features']

    def classify(values):
        if values.size == 0 or not np.isfinite(values).all() or np.any(values < c['input_min']) or np.any(values > c['input_max']):
            raise ValueError('Python invalid batch')
        normalized = np.zeros((batch, features), dtype=np.float32)
        normalized[:len(values)] = ((values.astype(np.float64) - c['offset']) * c['scale']).astype(np.float32)
        scores = session.run([output_name], {name: normalized})[0]
        if scores.shape != (batch, classes) or scores.dtype != np.float32 or not np.isfinite(scores).all():
            raise ValueError('Python invalid output')
        scores = scores[:len(values)].copy()
        ranked = np.argsort(-scores, axis=1, kind='stable')[:, :c['top_k']]
        return scores, ranked[:, 0].copy(), ranked.copy()

    start = time.perf_counter()
    for _ in range(q['warmups']):
        classify(raw[:batch])
    warmup_ms = (time.perf_counter() - start) * 1000
    trials, reference, prediction, topk = [], None, None, None
    for _ in range(q['trials']):
        unix_start, start = time.time(), time.perf_counter()
        completed = 0
        for _ in range(q['repetitions']):
            score_parts, label_parts, ranked_parts = [], [], []
            for offset in range(0, len(raw), batch):
                scores, predicted, ranked = classify(raw[offset:offset + batch])
                score_parts.append(scores)
                label_parts.append(predicted)
                ranked_parts.append(ranked)
                completed += len(predicted)
            scores, predicted, ranked = np.concatenate(score_parts), np.concatenate(label_parts), np.concatenate(ranked_parts)
            if reference is None:
                reference, prediction, topk = scores.copy(), predicted.copy(), ranked.copy()
            if not np.array_equal(predicted, prediction) or not np.allclose(scores, reference, atol=q.get('absolute_tolerance', 1e-5), rtol=q.get('relative_tolerance', 1e-4)):
                raise ValueError('Python numerical regression')
        elapsed, unix_end = (time.perf_counter() - start) * 1000, time.time()
        accuracy = float(np.mean(predicted == labels))
        if accuracy < c['minimum_accuracy']:
            raise ValueError('Python quality below threshold')
        trials.append(dict(success=True, elapsed_ms=elapsed, completed=completed, accuracy=accuracy,
                           start_unix_seconds=unix_start, end_unix_seconds=unix_end))
    report.update(runner='python_numpy_onnxruntime_cpu', backend_version=ort.__version__, python_version=sys.version.split()[0],
                  numpy_version=np.__version__, baseline_source_sha256=sha(__file__), success=True, rows=len(raw),
                  accuracy=float(np.mean(prediction == labels)), top_k_accuracy=float(np.mean(np.any(topk == labels[:, None], axis=1))),
                  scores=reference.flatten().tolist(), predictions=prediction.tolist(), preparation_ms=prep_ms,
                  warmup_ms=warmup_ms, trials=trials, comparative_energy_claim=False)
    write(output, report)


def validate_pair(native, python, protocol, require_energy=False):
    keys = ('configuration_sha256', 'qualification_sha256', 'model_sha256', 'dataset_sha256', 'dataset_id', 'dataset_split',
            'hardware_fingerprint', 'compiler_revision', 'precision', 'backend', 'backend_version', 'threads', 'batch_size',
            'functional_unit', 'boundary', 'rows', 'minimum_accuracy')
    if not native['success'] or not python['success'] or any(native[k] != python[k] for k in keys):
        raise ValueError('baseline workload or environment mismatch')
    if native['predictions'] != python['predictions'] or len(native['predictions']) != native['rows']:
        raise ValueError('baseline prediction mismatch')
    if len(native['scores']) != len(python['scores']) or not native['scores']:
        raise ValueError('baseline score shape mismatch')
    for a, b in zip(native['scores'], python['scores']):
        if not math.isfinite(a) or not math.isfinite(b) or abs(a-b) > protocol.get('absolute_tolerance', 1e-5) + protocol.get('relative_tolerance', 1e-4) * abs(a):
            raise ValueError('baseline numerical mismatch')
    energy = True
    for report in (native, python):
        if not math.isfinite(report['accuracy']) or not report['minimum_accuracy'] <= report['accuracy'] <= 1:
            raise ValueError('baseline quality failure')
        if len(report['trials']) != protocol['trials'] or len(report['trials']) < 3:
            raise ValueError('baseline requires complete repeated trials')
        for trial in report['trials']:
            if trial['success'] is not True or type(trial['completed']) is not int or trial['completed'] != report['rows'] * protocol['repetitions'] or not math.isfinite(trial['elapsed_ms']) or trial['elapsed_ms'] <= 0 or not math.isfinite(trial['accuracy']) or not report['minimum_accuracy'] <= trial['accuracy'] <= 1:
                raise ValueError('baseline incomplete trial')
            measured = trial.get('energy', {})
            valid = (measured.get('claim_eligible') is True and measured.get('available') is True and measured.get('evidence_class') == 'measured'
                     and measured.get('source_kind') == 'physical_meter' and measured.get('functional_units') == trial['completed']
                     and measured.get('source_sample_count', 0) >= 11 and measured.get('joules_per_fu', 0) > 0
                     and measured.get('instrument', {}).get('uncertainty_percent', 101) <= protocol.get('maximum_uncertainty_percent', 20))
            if valid:
                valid = (all(math.isfinite(measured[k]) and measured[k] > 0 for k in ('joules', 'joules_per_fu', 'elapsed_seconds', 'average_watts'))
                         and math.isclose(measured['joules_per_fu'] * trial['completed'], measured['joules'], rel_tol=1e-9)
                         and math.isclose(measured['average_watts'] * measured['elapsed_seconds'], measured['joules'], rel_tol=1e-9)
                         and abs(measured['start_unix_seconds'] - trial['start_unix_seconds']) <= .01
                         and abs(measured['end_unix_seconds'] - trial['end_unix_seconds']) <= .01)
            energy = energy and valid
    if require_energy and not energy:
        raise ValueError('calibrated, sufficiently sampled paired energy evidence required')
    if energy:
        a, b = native['trials'][0]['energy']['instrument'], python['trials'][0]['energy']['instrument']
        if a != b or any(t['energy']['instrument'] != a for r in (native, python) for t in r['trials']):
            raise ValueError('baseline instrument or boundary mismatch')
    return energy


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tool', required=True)
    parser.add_argument('--config', type=pathlib.Path, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--require-energy', action='store_true')
    parser.add_argument('--pairs', type=int, default=4, help='even number of alternating runner pairs, 4 to 10')
    parser.add_argument('--policy', type=pathlib.Path, help='engineering policy frozen before execution')
    parser.add_argument('--python-worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.python_worker:
        worker(args.config, args.output, args.tool)
        return
    from assess_ai_application_comparison import validate_policy
    if args.pairs < 4 or args.pairs > 10 or args.pairs % 2:
        parser.error('comparison requires 4 to 10 balanced runner pairs')
    args.output.mkdir(parents=True, exist_ok=False)
    policy_path = args.policy or pathlib.Path(__file__).resolve().parents[1] / 'tests/ai_application' / (
        'comparison_measurement_policy.json' if args.require_energy else 'comparison_execution_policy.json')
    policy = load(policy_path)
    validate_policy(policy)
    if (policy['mode'] == 'calibrated_energy') != args.require_energy or args.pairs < policy['minimum_pairs']:
        parser.error('execution mode and pair count must meet the declared policy')
    c = load(args.config)
    q = load(c['qualification_config'])
    inputs = {'application-config.json': args.config, 'qualification-config.json': pathlib.Path(c['qualification_config']),
              'policy.json': policy_path}
    for name, path in inputs.items():
        (args.output / name).write_bytes(path.read_bytes())
    pinned = {name: sha(args.output / name) for name in inputs}
    native_binary = sha(args.tool)
    started = time.time()
    if args.require_energy and (q.get('energy_source') != 'physical_meter' or not q.get('require_measured_energy')):
        parser.error('energy comparison requires the calibrated physical-meter protocol')
    pairs, energy_values = [], {'native': [], 'python': []}
    for index in range(args.pairs):
        paths = {key: args.output / f'{index}-{key}.json' for key in ('native', 'python')}
        order = ('native', 'python') if index % 2 == 0 else ('python', 'native')
        for key in order:
            process_start = time.perf_counter()
            if key == 'native':
                execute([args.tool, 'application', args.config, paths[key]])
            else:
                execute([sys.executable, __file__, '--python-worker', '--tool', args.tool, '--config', args.config, '--output', paths[key]])
            report = load(paths[key])
            report['process_elapsed_ms'] = (time.perf_counter() - process_start) * 1000
            write(paths[key], report)
        pairs.append(dict(order=list(order), native_file=paths['native'].name, python_file=paths['python'].name))
    raw_trace = None
    if q.get('energy_source') == 'physical_meter':
        source = pathlib.Path(q['meter_csv'])
        if source.is_symlink() or not source.is_file() or source.stat().st_size > 32 * 1024 * 1024:
            raise ValueError('unsafe physical trace')
        with source.open('rb') as stream:
            data = stream.read(32 * 1024 * 1024 + 1)
        if len(data) > 32 * 1024 * 1024 or not data.endswith(b'\n'):
            raise ValueError('incomplete or oversized physical trace snapshot')
        (args.output / 'meter.csv').write_bytes(data)
        write(args.output / 'instrument.json', q['instrument'])
        raw_trace = dict(file='meter.csv', sha256=sha(args.output / 'meter.csv'),
                         instrument_file='instrument.json', instrument_sha256=sha(args.output / 'instrument.json'))
    # Trial windows share one immutable snapshot. Other phases stay separate.
    for index, pair in enumerate(pairs):
        paths = {key: args.output / pair[key + '_file'] for key in ('native', 'python')}
        if raw_trace:
            for key in paths:
                report = load(paths[key])
                for n, trial in enumerate(report['trials']):
                    measurement = args.output / f'{index}-{key}-meter-{n}.json'
                    execute([args.tool, 'meter-window', args.output / 'meter.csv', args.output / 'instrument.json',
                             trial['start_unix_seconds'], trial['end_unix_seconds'], trial['completed'], measurement])
                    trial['energy'] = load(measurement)
                write(paths[key], report)
        a, b = (load(paths[key]) for key in ('native', 'python'))
        energy = validate_pair(a, b, q, args.require_energy)
        if energy:
            for key, report in [('native', a), ('python', b)]:
                energy_values[key].extend(t['energy']['joules_per_fu'] for t in report['trials'])
        pair.update(dict(native_sha256=sha(paths['native']), python_sha256=sha(paths['python']), energy_comparison_valid=energy,
                          native_ms_per_image=[t['elapsed_ms']/t['completed'] for t in a['trials']],
                          python_ms_per_image=[t['elapsed_ms']/t['completed'] for t in b['trials']]))
    if any(sha(path) != pinned[name] for name, path in inputs.items()) or sha(args.tool) != native_binary:
        raise ValueError('comparison inputs changed during execution')
    native_ms = [v for p in pairs for v in p['native_ms_per_image']]
    python_ms = [v for p in pairs for v in p['python_ms_per_image']]
    write(args.output / 'comparison.json', dict(schema='shorthand.ai.application.comparison.v2', success=True,
          inputs=pinned, native_binary_sha256=native_binary, baseline_source_sha256=sha(__file__),
          capture_started_unix_seconds=started, capture_finished_unix_seconds=time.time(), raw_trace=raw_trace,
          scope='native host AIRuntime application versus optimized Python ONNX; not a compiler or whole-language ranking',
          pairs=pairs, mean_native_ms_per_image=statistics.mean(native_ms), mean_python_ms_per_image=statistics.mean(python_ms),
          observed_native_over_python_latency_ratio=statistics.mean(native_ms)/statistics.mean(python_ms),
          measured_joules_per_image=energy_values if all(p['energy_comparison_valid'] for p in pairs) else None,
          energy_comparison_valid=all(p['energy_comparison_valid'] for p in pairs), comparative_energy_claim=False,
          carbon_superiority_claim=False, official_certification_granted=False))
    print('PASS paired native/Python accuracy, numerical agreement and repeated boundary-matched execution')

if __name__ == '__main__':
    main()
