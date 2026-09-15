#!/usr/bin/env python3
"""Replay CPU comparison evidence against a predeclared engineering policy.

Hashes establish integrity, not authenticity, independent execution, certification,
carbon superiority or a whole-language ranking.
"""
import argparse
import datetime
import math
import pathlib
import statistics
import tempfile

from compare_ai_application_baselines import execute, load, sha, validate_pair, write


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def number(value, low=0, high=1e15, integer=False):
    require(type(value) in (int, float) and math.isfinite(value) and low <= value <= high,
            'invalid finite numeric evidence')
    require(not integer or type(value) is int, 'integer evidence required')
    return value


def digest(value):
    require(isinstance(value, str) and len(value) == 64 and all(c in '0123456789abcdef' for c in value), 'invalid SHA-256')
    return value


def validate_policy(policy):
    limits = {
        'minimum_pairs': (4, 10, True), 'minimum_window_seconds': (0.01, 3600, False),
        'minimum_source_samples': (11, 1000000, True), 'maximum_sample_gap_seconds': (1e-6, 60, False),
        'maximum_relative_sample_gap': (1e-6, .1, False), 'maximum_clock_skew_seconds': (1e-6, .01, False),
        'maximum_calibration_age_days': (1, 3650, True), 'maximum_uncertainty_percent': (0, 50, False),
        'maximum_native_ms_per_image': (1e-9, 60000, False), 'maximum_preparation_ms': (1e-6, 1800000, False),
        'maximum_warmup_ms': (1e-6, 1800000, False), 'maximum_energy_joules_per_image': (1e-9, 1e9, False),
        'maximum_regression_percent': (0, 100, False), 'maximum_trial_variability_percent': (0, 50, False),
    }
    require(isinstance(policy, dict) and set(policy) == set(limits) | {'schema', 'mode', 'instrument_boundary'},
            'unexpected or missing policy field')
    require(policy['schema'] == 'shorthand.ai.comparison.policy.v1', 'unsupported comparison policy')
    require(policy['mode'] in ('execution_only', 'calibrated_energy'), 'invalid policy mode')
    for key, (low, high, integer) in limits.items():
        number(policy[key], low, high, integer)
    require(policy['minimum_pairs'] % 2 == 0, 'policy requires balanced pairs')
    require(isinstance(policy['instrument_boundary'], str) and 1 <= len(policy['instrument_boundary']) <= 128
            and policy['instrument_boundary'].isprintable(), 'invalid instrument boundary')


def artifact(directory, name, expected):
    require(isinstance(name, str) and name not in ('', '.', '..') and '/' not in name and '\\' not in name,
            'unsafe bundle artifact name')
    path = directory / name
    require(not path.is_symlink() and path.is_file() and path.stat().st_size <= 32 * 1024 * 1024,
            'unsafe or oversized bundle artifact')
    require(sha(path) == digest(expected), 'bundle artifact digest mismatch: ' + name)
    return path


def summary(values):
    require(bool(values), 'missing observations')
    for value in values:
        number(value, 1e-15)
    average = statistics.mean(values)
    deviation = statistics.stdev(values) if len(values) > 1 else 0
    ordered = sorted(values)
    return dict(raw=values, mean=average, median=statistics.median(values),
                p95=ordered[math.ceil(.95 * len(values)) - 1], sample_standard_deviation=deviation,
                variability_percent=100 * deviation / average)


def replay_measurement(tool, trace, instrument, trial, recorded, scratch):
    output = scratch / 'replayed.json'
    execute([tool, 'meter-window', trace, instrument, trial['start_unix_seconds'], trial['end_unix_seconds'],
             trial['completed'], output])
    # Reuse native parsing, interpolation and integration; no Python integrator.
    require(load(output) == recorded, 'native energy replay mismatch')


def validate_measurement(trial, measured, policy, trace_hash):
    require(measured['available'] is True and measured['claim_eligible'] is True
            and measured['source_kind'] == 'physical_meter' and measured['evidence_class'] == 'measured',
            'calibrated physical measurement required')
    require(measured['trace_sha256'] == trace_hash, 'energy trace identity mismatch')
    duration = number(measured['elapsed_seconds'], policy['minimum_window_seconds'], 1800)
    number(measured['source_sample_count'], policy['minimum_source_samples'], 1000000, True)
    gap = number(measured['maximum_source_gap_seconds'], 1e-9, 1800)
    epsilon = 2 * math.ulp(max(trial['start_unix_seconds'], trial['end_unix_seconds']))
    require(gap <= policy['maximum_sample_gap_seconds'] + epsilon, 'source sampling gap exceeds policy')
    require(gap <= duration * policy['maximum_relative_sample_gap'] + epsilon, 'sparse source sampling')
    instrument = measured['instrument']
    require(instrument['boundary'] == policy['instrument_boundary'], 'instrument boundary mismatch')
    number(instrument['uncertainty_percent'], 0, policy['maximum_uncertainty_percent'])
    calibrated = datetime.date.fromisoformat(instrument['calibration_date'])
    measured_date = datetime.datetime.fromtimestamp(trial['end_unix_seconds'], datetime.timezone.utc).date()
    require(0 <= (measured_date - calibrated).days <= policy['maximum_calibration_age_days'], 'expired or future calibration')
    for key in ('id', 'calibration_id', 'validation_ref', 'isolation'):
        require(isinstance(instrument[key], str) and instrument[key].strip(), 'missing instrument provenance')


def assess_bundle(directory, expected_manifest, expected_policy, tool):
    directory = pathlib.Path(directory)
    require(not directory.is_symlink() and directory.is_dir(), 'unsafe bundle directory')
    manifest = load(artifact(directory, 'comparison.json', expected_manifest))
    require(manifest['schema'] == 'shorthand.ai.application.comparison.v2' and manifest['success'] is True,
            'complete v2 comparison required')
    for key in ('comparative_energy_claim', 'carbon_superiority_claim', 'official_certification_granted'):
        require(manifest[key] is False, 'comparison must not grant a public claim')
    inputs = manifest['inputs']
    require(set(inputs) == {'application-config.json', 'qualification-config.json', 'policy.json'}, 'incomplete comparison input snapshot')
    require(inputs['policy.json'] == digest(expected_policy), 'predeclared policy digest mismatch')
    policy = load(artifact(directory, 'policy.json', expected_policy))
    validate_policy(policy)
    c = load(artifact(directory, 'application-config.json', inputs['application-config.json']))
    q = load(artifact(directory, 'qualification-config.json', inputs['qualification-config.json']))
    require(c['qualification_sha256'] == inputs['qualification-config.json'], 'protocol snapshot mismatch')
    require(c['schema'] == 'shorthand.ai.application.config.v1' and q['schema'] == 'shorthand.ai.cpu_qualification.config.v1',
            'invalid workload configuration schema')
    number(q['repetitions'], 1, 10000, True)
    number(q['trials'], 3, 25, True)
    number(c['classes'], 2, 256, True)
    number(c['features'], 1, 4096, True)
    number(c['minimum_accuracy'], 1e-9, 1)
    require_energy = policy['mode'] == 'calibrated_energy'
    require(not require_energy or (q.get('require_measured_energy') is True and q['energy_source'] == 'physical_meter'),
            'measurement protocol must require physical energy')
    pairs = manifest['pairs']
    require(isinstance(pairs, list) and policy['minimum_pairs'] <= len(pairs) <= 10 and len(pairs) % 2 == 0,
            'balanced repeated pairs required')
    begin = number(manifest['capture_started_unix_seconds'], 946684800, 4102444800)
    finish = number(manifest['capture_finished_unix_seconds'], begin, 4102444800)
    require(finish > begin, 'invalid capture window')
    for key in ('native_binary_sha256', 'baseline_source_sha256'):
        digest(manifest[key])
    raw = manifest['raw_trace']
    trace = instrument_path = None
    if raw is not None:
        require(set(raw) == {'file', 'sha256', 'instrument_file', 'instrument_sha256'}, 'invalid trace manifest')
        trace = artifact(directory, raw['file'], raw['sha256'])
        instrument_path = artifact(directory, raw['instrument_file'], raw['instrument_sha256'])
        require(load(instrument_path) == q['instrument'], 'instrument snapshot mismatch')
    require(not require_energy or trace is not None, 'raw physical trace required')
    timings = {key: [] for key in ('native', 'python')}
    joules = {key: [] for key in timings}
    pair_latency, pair_energy, failures, reports, identities = [], [], [], [], set()
    previous_end, common, metric_instrument = begin, None, None
    with tempfile.TemporaryDirectory(prefix='shorthand-comparison-replay-') as temp:
        for index, pair in enumerate(pairs):
            require(pair['order'] == (['native', 'python'] if index % 2 == 0 else ['python', 'native']),
                    'runner order must alternate without dropped pairs')
            current = {}
            for runner in pair['order']:
                name = pair[runner + '_file']
                require(name not in identities, 'reused runner artifact')
                identities.add(name)
                report = load(artifact(directory, name, pair[runner + '_sha256']))
                require(report['schema'] == 'shorthand.ai.application.report.v1' and report['success'] is True,
                        'complete successful runner report required')
                require(report['runner'] == ('shorthand_native_application' if runner == 'native' else 'python_numpy_onnxruntime_cpu'),
                        'runner identity mismatch')
                require(report['configuration_sha256'] == inputs['application-config.json']
                        and report['qualification_sha256'] == inputs['qualification-config.json'], 'runner configuration mismatch')
                require(report['model_sha256'] == q['model_sha256'] and report['dataset_sha256'] == c['dataset_sha256']
                        and report['minimum_accuracy'] == c['minimum_accuracy'] and report['batch_size'] == q['batch_size']
                        and report['threads'] == c['threads'], 'runner workload mismatch')
                number(report['rows'], c['classes'], 100000, True)
                require(len(report['scores']) == report['rows'] * c['classes'] <= 262144, 'score tensor size mismatch')
                for prediction in report['predictions']:
                    number(prediction, 0, c['classes'] - 1, True)
                for flag in ('production_claim', 'comparative_energy_claim', 'official_certification_granted'):
                    require(report[flag] is False, 'runner must not grant a public claim')
                number(report['process_elapsed_ms'], 1e-9, 1800000)
                for field, limit in [('preparation_ms', 'maximum_preparation_ms'), ('warmup_ms', 'maximum_warmup_ms')]:
                    number(report[field], 0, 1800000)
                    if report[field] > policy[limit]:
                        failures.append(runner + '_' + limit)
                key = {name: report[name] for name in ('hardware_fingerprint', 'backend', 'backend_version', 'precision',
                       'boundary', 'functional_unit', 'rows', 'compiler_revision')}
                require(common is None or common == key, 'environment changed between runner pairs')
                common = key
                require(len(report['trials']) == q['trials'], 'incomplete runner trials')
                for trial in report['trials']:
                    start = number(trial['start_unix_seconds'], begin, finish)
                    end = number(trial['end_unix_seconds'], start, finish)
                    elapsed = number(trial['elapsed_ms'], 1e-9, 1800000) / 1000
                    require(start >= previous_end and end > start, 'overlapping or reordered execution windows')
                    previous_end = end
                    require(abs(end - start - elapsed) <= policy['maximum_clock_skew_seconds'], 'wall/monotonic clock mismatch')
                    number(trial['completed'], 1, 50000000, True)
                    timings[runner].append(trial['elapsed_ms'] / trial['completed'])
                    if trace is not None:
                        measured = trial['energy']
                        replay_measurement(tool, trace, instrument_path, trial, measured, pathlib.Path(temp))
                        if require_energy:
                            validate_measurement(trial, measured, policy, raw['sha256'])
                            require(metric_instrument is None or metric_instrument == measured['instrument'], 'instrument changed between pairs')
                            metric_instrument = measured['instrument']
                            joules[runner].append(number(measured['joules_per_fu'], 1e-15))
                require(report['process_elapsed_ms'] + policy['maximum_clock_skew_seconds'] * 1000 >=
                        report['preparation_ms'] + report['warmup_ms'] + sum(t['elapsed_ms'] for t in report['trials']),
                        'process timing does not contain measured phases')
                current[runner] = report
                reports.append({key: report.get(key) for key in ('runner', 'python_version', 'numpy_version', 'baseline_source_sha256')})
            energy = validate_pair(current['native'], current['python'], q, require_energy)
            require(pair['energy_comparison_valid'] is energy, 'pair energy status mismatch')
            for runner in current:
                values = [t['elapsed_ms'] / t['completed'] for t in current[runner]['trials']]
                require(pair[runner + '_ms_per_image'] == values, 'derived latency values mismatch')
            pair_latency.append(statistics.mean(pair['native_ms_per_image']) / statistics.mean(pair['python_ms_per_image']))
            if require_energy:
                pair_energy.append(statistics.mean(t['energy']['joules_per_fu'] for t in current['native']['trials']) /
                                   statistics.mean(t['energy']['joules_per_fu'] for t in current['python']['trials']))
    latency = {key: summary(values) for key, values in timings.items()}
    require(manifest['energy_comparison_valid'] is all(p['energy_comparison_valid'] for p in pairs), 'aggregate energy status mismatch')
    if not manifest['energy_comparison_valid']:
        require(manifest['measured_joules_per_image'] is None, 'unqualified aggregate energy values')
    require(manifest['mean_native_ms_per_image'] == latency['native']['mean']
            and manifest['mean_python_ms_per_image'] == latency['python']['mean']
            and manifest['observed_native_over_python_latency_ratio'] == latency['native']['mean'] / latency['python']['mean'],
            'derived comparison summary mismatch')
    if latency['native']['p95'] > policy['maximum_native_ms_per_image']:
        failures.append('native_latency_budget')
    energy_summary = None
    if require_energy:
        require(manifest['energy_comparison_valid'] is True and manifest['measured_joules_per_image'] == joules,
                'energy comparison summary mismatch')
        energy_summary = {key: summary(values) for key, values in joules.items()}
        for runner, observation in energy_summary.items():
            observation['conservative_uncertainty_percent'] = metric_instrument['uncertainty_percent'] + 2 * observation['variability_percent']
            if observation['conservative_uncertainty_percent'] > policy['maximum_uncertainty_percent']:
                failures.append(runner + '_energy_uncertainty')
            if latency[runner]['variability_percent'] > policy['maximum_trial_variability_percent']:
                failures.append(runner + '_latency_variability')
        if energy_summary['native']['mean'] > policy['maximum_energy_joules_per_image']:
            failures.append('native_energy_budget')
    # Native revisions may change; workload, baseline, environment and instrument may not.
    contract = dict(application={key: value for key, value in c.items() if key not in
        ('qualification_config', 'qualification_sha256', 'dataset_path')},
        protocol={key: value for key, value in q.items() if key not in ('model_path', 'meter_csv')},
        environment={key: value for key, value in common.items() if key != 'compiler_revision'},
        baseline_source_sha256=manifest['baseline_source_sha256'],
        python_version=current['python']['python_version'], numpy_version=current['python']['numpy_version'])
    require(all(r.get('python_version') == contract['python_version'] and r.get('numpy_version') == contract['numpy_version']
                and r.get('baseline_source_sha256') == manifest['baseline_source_sha256']
                for r in reports if r['runner'] == 'python_numpy_onnxruntime_cpu'), 'Python baseline environment mismatch')
    return dict(schema='shorthand.ai.comparison.assessment.v1', success=not failures, failures=sorted(set(failures)),
                mode=policy['mode'], policy_sha256=expected_policy, comparison_sha256=expected_manifest,
                contract=contract, latency_ms_per_image=latency, energy_joules_per_image=energy_summary,
                paired_native_over_python_latency_ratio=summary(pair_latency),
                paired_native_over_python_energy_ratio=summary(pair_energy) if pair_energy else None,
                energy_evidence_qualified=require_energy and not failures, regression=None,
                uncertainty_method='instrument uncertainty plus twice sample standard deviation divided by mean; not a confidence interval',
                scope='resident-data CPU host execution; preparation, warmup and total process time are separate; no whole-language rank',
                assessor_sha256=sha(__file__), replay_binary_sha256=sha(tool), comparative_energy_claim=False,
                carbon_superiority_claim=False, official_certification_granted=False)


def compare_previous(current, previous, policy, previous_hash):
    require(previous['success'] and current['contract'] == previous['contract'] and current['mode'] == previous['mode'],
            'previous evidence must pass with the same workload, environment and policy mode')
    a, b = current['latency_ms_per_image']['native'], previous['latency_ms_per_image']['native']
    ratio = a['mean'] / b['mean']
    result = dict(previous_comparison_sha256=previous_hash, native_latency_ratio=ratio)
    limit = 1 + policy['maximum_regression_percent'] / 100
    if max(a['variability_percent'], b['variability_percent']) > policy['maximum_trial_variability_percent'] or b['variability_percent'] >= 50:
        current['failures'].append('latency_regression_unstable_measurements')
    else:
        upper = ratio * (1 + 2 * a['variability_percent'] / 100) / (1 - 2 * b['variability_percent'] / 100)
        result['conservative_native_latency_ratio'] = upper
        if upper > limit:
            current['failures'].append('latency_regression')
    if current['mode'] == 'calibrated_energy':
        a, b = current['energy_joules_per_image']['native'], previous['energy_joules_per_image']['native']
        upper = a['mean'] * (1 + a['conservative_uncertainty_percent'] / 100) / (
            b['mean'] * (1 - b['conservative_uncertainty_percent'] / 100))
        result.update(native_energy_ratio=a['mean'] / b['mean'], conservative_native_energy_ratio=upper)
        if upper > limit:
            current['failures'].append('energy_regression_or_insufficient_margin')
    current['regression'] = result
    current['success'] = not current['failures']
    current['energy_evidence_qualified'] = current['mode'] == 'calibrated_energy' and current['success']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle', type=pathlib.Path, required=True)
    parser.add_argument('--comparison-sha256', required=True)
    parser.add_argument('--policy-sha256', required=True)
    parser.add_argument('--tool', required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--previous', type=pathlib.Path)
    parser.add_argument('--previous-sha256')
    args = parser.parse_args()
    try:
        require(bool(args.previous) == bool(args.previous_sha256), 'previous bundle and trusted digest required together')
        result = assess_bundle(args.bundle, args.comparison_sha256, args.policy_sha256, args.tool)
        if args.previous:
            previous = assess_bundle(args.previous, args.previous_sha256, args.policy_sha256, args.tool)
            compare_previous(result, previous, load(args.bundle / 'policy.json'), args.previous_sha256)
    except (ValueError, KeyError, TypeError, OSError, RuntimeError, OverflowError, RecursionError) as error:
        result = dict(schema='shorthand.ai.comparison.assessment.v1', success=False, failures=[str(error)],
                      energy_evidence_qualified=False, comparative_energy_claim=False, carbon_superiority_claim=False,
                      official_certification_granted=False)
    write(args.output, result)
    if not result['success']:
        raise SystemExit(2)
    print('PASS comparison integrity, replay and declared engineering policy; no certification or carbon claim')


if __name__ == '__main__':
    main()
