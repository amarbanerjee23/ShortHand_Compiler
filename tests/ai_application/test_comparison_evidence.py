"""Synthetic validator fixtures only; never retained as workload measurements."""
import copy
import json
import pathlib
import shutil
import statistics
import subprocess
import sys

root, tool, work = map(pathlib.Path, sys.argv[1:4])
sys.path.insert(0, str(root / 'scripts'))
from assess_ai_application_comparison import assess_bundle, compare_previous, validate_policy
from compare_ai_application_baselines import sha, write

work = work / 'comparison-evidence'
work.mkdir()
checks = 0
instrument = dict(id='synthetic-validator-fixture', calibration_id='not-a-real-calibration', calibration_date='2026-01-01',
                  validation_ref='synthetic_test_only', boundary='whole_host_ac', isolation='unit_test_only',
                  uncertainty_percent=1, maximum_power_w=100)


def native(*args, good=True):
    result = subprocess.run([str(tool), *map(str, args)], text=True, capture_output=True, timeout=120)
    assert result.returncode == (0 if good else 2), result.stderr
    assert not any(x in result.stderr for x in ('AddressSanitizer', 'LeakSanitizer', 'runtime error:')), result.stderr
    return result


def fixture(name, mode='calibrated_energy', duration=1, calibration='2026-01-01', variable=False, sample_step=.01):
    directory = work / name
    directory.mkdir()
    p = json.loads((root / 'tests/ai_application/comparison_measurement_policy.json').read_text())
    p.update(mode=mode, maximum_native_ms_per_image=1000)
    write(directory / 'policy.json', p)
    i = dict(instrument, calibration_date=calibration)
    write(directory / 'instrument.json', i)
    q = dict(schema='shorthand.ai.cpu_qualification.config.v1', model_path='/fixture/model.onnx', model_sha256='a' * 64,
             input_shape=[2, 2], output_shape=[2, 2], threads=[1], batch_size=2, warmups=2, repetitions=2, trials=3,
             energy_source='physical_meter' if mode == 'calibrated_energy' else 'unavailable',
             require_measured_energy=mode == 'calibrated_energy', instrument=i, meter_csv='/fixture/meter.csv')
    write(directory / 'qualification-config.json', q)
    c = dict(schema='shorthand.ai.application.config.v1', qualification_config='/fixture/qualification.json',
             qualification_sha256=sha(directory / 'qualification-config.json'), dataset_path='/fixture/data.csv',
             dataset_sha256='b' * 64, dataset_id='synthetic_validator_fixture', dataset_source='unit_test_only',
             dataset_license='repository_test_fixture', dataset_split='synthetic', features=2, classes=2, top_k=1,
             threads=1, workers=1, request_timeout_ms=30000, input_min=0, input_max=1, offset=0, scale=1, minimum_accuracy=.95)
    write(directory / 'application-config.json', c)
    base = 1780000000
    trace = directory / 'meter.csv'
    trace.write_text('unix_time_s,power_w\n' + ''.join(f'{base - 1 + n * sample_step:.2f},20\n' for n in range(int(100 / sample_step))))
    pairs, all_values, energy_values = [], dict(native=[], python=[]), dict(native=[], python=[])
    sequence = 0
    for pair_id in range(4):
        pair = dict(order=['native', 'python'] if pair_id % 2 == 0 else ['python', 'native'])
        for runner in pair['order']:
            trials = []
            for trial_id in range(3):
                elapsed = (duration if runner == 'native' else 1) * (1.5 if variable and trial_id == 2 else 1)
                start = base + sequence * 3
                sequence += 1
                trial = dict(success=True, elapsed_ms=elapsed * 1000, start_unix_seconds=start,
                             end_unix_seconds=start + elapsed, completed=4, accuracy=1)
                if mode == 'calibrated_energy':
                    output = directory / 'measurement.json'
                    native('meter-window', trace, directory / 'instrument.json', start, start + elapsed, 4, output)
                    trial['energy'] = json.loads(output.read_text())
                    energy_values[runner].append(trial['energy']['joules_per_fu'])
                trials.append(trial)
            values = [t['elapsed_ms'] / t['completed'] for t in trials]
            all_values[runner].extend(values)
            report = dict(schema='shorthand.ai.application.report.v1', configuration_sha256=sha(directory / 'application-config.json'),
                          qualification_sha256=sha(directory / 'qualification-config.json'), model_sha256=q['model_sha256'],
                          dataset_sha256=c['dataset_sha256'], dataset_id=c['dataset_id'], dataset_split=c['dataset_split'],
                          hardware_fingerprint='synthetic_unit_fixture', compiler_revision='fixture_revision', precision='float32',
                          backend='onnxruntime_cpu', backend_version='1.30.0', threads=1, batch_size=2,
                          functional_unit='completed_classification', boundary='unit fixture only', rows=2,
                          minimum_accuracy=.95, accuracy=1, scores=[1, 0, 0, 1], predictions=[0, 1], trials=trials, success=True,
                          preparation_ms=1, warmup_ms=1, process_elapsed_ms=10000,
                          runner='shorthand_native_application' if runner == 'native' else 'python_numpy_onnxruntime_cpu',
                          python_version='3.12.fixture', numpy_version='fixture', baseline_source_sha256=sha(root / 'scripts/compare_ai_application_baselines.py'),
                          production_claim=False, comparative_energy_claim=False, official_certification_granted=False)
            filename = f'{pair_id}-{runner}.json'
            write(directory / filename, report)
            pair.update({runner + '_file': filename, runner + '_sha256': sha(directory / filename), runner + '_ms_per_image': values})
        pair['energy_comparison_valid'] = mode == 'calibrated_energy'
        pairs.append(pair)
    a, b = (statistics.mean(all_values[key]) for key in ('native', 'python'))
    manifest = dict(schema='shorthand.ai.application.comparison.v2', success=True, pairs=pairs,
                    inputs={name: sha(directory / name) for name in ('application-config.json', 'qualification-config.json', 'policy.json')},
                    native_binary_sha256=sha(tool), baseline_source_sha256=sha(root / 'scripts/compare_ai_application_baselines.py'),
                    capture_started_unix_seconds=base - .5, capture_finished_unix_seconds=base + 80,
                    raw_trace=dict(file='meter.csv', sha256=sha(trace), instrument_file='instrument.json',
                                   instrument_sha256=sha(directory / 'instrument.json')) if mode == 'calibrated_energy' else None,
                    mean_native_ms_per_image=a, mean_python_ms_per_image=b, observed_native_over_python_latency_ratio=a / b,
                    measured_joules_per_image=energy_values if mode == 'calibrated_energy' else None,
                    energy_comparison_valid=mode == 'calibrated_energy', comparative_energy_claim=False,
                    carbon_superiority_claim=False, official_certification_granted=False)
    write(directory / 'comparison.json', manifest)
    return directory


def assess(directory, **kwargs):
    return assess_bundle(directory, kwargs.get('manifest', sha(directory / 'comparison.json')),
                         kwargs.get('policy', sha(directory / 'policy.json')), tool)


def expect_failure(fn, reason=''):
    global checks
    checks += 1
    try:
        result = fn()
    except (ValueError, RuntimeError, KeyError, TypeError):
        return
    assert result.get('success') is False and any(reason in x for x in result['failures']), result


good = fixture('valid')
passed = assess(good)
assert passed['success'] and passed['energy_evidence_qualified']
assert passed['energy_joules_per_image']['native']['mean'] == 5
assert passed['paired_native_over_python_energy_ratio']['mean'] == 1
assert not passed['comparative_energy_claim'] and not passed['official_certification_granted']
checks += 1
execution = fixture('execution', mode='execution_only')
assert assess(execution)['success'] and not assess(execution)['energy_evidence_qualified']
checks += 1
slow = fixture('slower', duration=1.2)
observed = assess(slow)
assert observed['success'] and observed['paired_native_over_python_latency_ratio']['mean'] > 1.19
checks += 1
policy = json.loads((good / 'policy.json').read_text())
same = copy.deepcopy(passed)
compare_previous(same, passed, policy, sha(good / 'comparison.json'))
assert same['success']
compare_previous(observed, passed, policy, sha(good / 'comparison.json'))
assert not observed['success'] and 'latency_regression' in observed['failures']
assert 'energy_regression_or_insufficient_margin' in observed['failures']
checks += 2
changed = copy.deepcopy(passed); changed['contract']['environment']['hardware_fingerprint'] = 'other'
expect_failure(lambda: compare_previous(changed, passed, policy, '0' * 64))
expect_failure(lambda: assess(good, manifest='0' * 64))
expect_failure(lambda: assess(good, policy='0' * 64))
expect_failure(lambda: assess(fixture('expired', calibration='2024-01-01')))
expect_failure(lambda: assess(fixture('short', duration=.05)))
expect_failure(lambda: assess(fixture('sparse', sample_step=.5)))
expect_failure(lambda: assess(fixture('source-gap', duration=1.5, sample_step=.11)))
expect_failure(lambda: assess(fixture('noisy', variable=True)), 'uncertainty')
for name, key, value in [('bool', 'minimum_pairs', True), ('odd', 'minimum_pairs', 5),
                         ('loose_gap', 'maximum_relative_sample_gap', .5), ('nan', 'maximum_uncertainty_percent', float('nan')),
                         ('clock', 'maximum_clock_skew_seconds', 1), ('unknown', 'unknown', 1)]:
    bad = dict(policy); bad[key] = value
    expect_failure(lambda bad=bad: validate_policy(bad))


def altered(name, change_report=None, change_manifest=None):
    directory = work / ('bad-' + name)
    shutil.copytree(good, directory)
    m = json.loads((directory / 'comparison.json').read_text())
    if change_report:
        path = directory / m['pairs'][0]['native_file']
        r = json.loads(path.read_text()); change_report(r); write(path, r)
        m['pairs'][0]['native_sha256'] = sha(path)
    if change_manifest:
        change_manifest(m, directory)
    write(directory / 'comparison.json', m)
    return directory


for name, change in [
    ('hardware', lambda r: r.update(hardware_fingerprint='different')),
    ('runner', lambda r: r.update(runner='python_numpy_onnxruntime_cpu')),
    ('prediction', lambda r: r['predictions'].__setitem__(0, True)),
    ('shape', lambda r: r['scores'].pop()),
    ('quality', lambda r: r.update(accuracy=.1)),
    ('claim', lambda r: r.update(production_claim=True)),
    ('trials', lambda r: r['trials'].pop()),
    ('clock', lambda r: r['trials'][0].update(elapsed_ms=100)),
    ('overlap', lambda r: r['trials'][1].update(start_unix_seconds=r['trials'][0]['start_unix_seconds'])),
    ('completed', lambda r: r['trials'][0].update(completed=3)),
    ('joules', lambda r: r['trials'][0]['energy'].update(joules=1)),
    ('sample_count', lambda r: r['trials'][0]['energy'].update(source_sample_count=999)),
    ('gap', lambda r: r['trials'][0]['energy'].update(maximum_source_gap_seconds=0)),
    ('trace_hash', lambda r: r['trials'][0]['energy'].update(trace_sha256='0' * 64)),
    ('startup', lambda r: r.update(preparation_ms=130000)),
]:
    expect_failure(lambda name=name, change=change: assess(altered(name, change_report=change)))
for name, change in [
    ('order', lambda m, d: m['pairs'][0].update(order=['python', 'native'])),
    ('missing_pair', lambda m, d: m['pairs'].pop()),
    ('repeat', lambda m, d: m['pairs'].__setitem__(2, copy.deepcopy(m['pairs'][0]))),
    ('traversal', lambda m, d: m['pairs'][0].update(native_file='../valid/0-native.json')),
    ('digest', lambda m, d: m['pairs'][0].update(native_sha256='0' * 64)),
    ('summary', lambda m, d: m.update(mean_native_ms_per_image=1)),
    ('raw_missing', lambda m, d: m.update(raw_trace=None)),
    ('public_claim', lambda m, d: m.update(carbon_superiority_claim=True)),
]:
    expect_failure(lambda name=name, change=change: assess(altered(name, change_manifest=change)))
symlink = altered('symlink')
path = symlink / '0-native.json'; path.unlink(); path.symlink_to(good / '0-native.json')
expect_failure(lambda: assess(symlink))
oversize = altered('oversize')
with (oversize / '0-native.json').open('wb') as stream:
    stream.truncate(32 * 1024 * 1024 + 1)
expect_failure(lambda: assess(oversize))
duplicate = altered('duplicate-key')
path = duplicate / '0-native.json'
path.write_text(path.read_text().replace('"success": true', '"success": true, "success": true', 1))
m = json.loads((duplicate / 'comparison.json').read_text())
m['pairs'][0]['native_sha256'] = sha(path); write(duplicate / 'comparison.json', m)
expect_failure(lambda: assess(duplicate))
native('meter-window', good / 'meter.csv', good / 'instrument.json', 1780000000, 1780000001, 1.5, work / 'invalid.json', good=False)
native('meter-window', good / 'meter.csv', good / 'instrument.json', 1780000000, 1780001000, 1, work / 'invalid.json', good=False)
checks += 2
cli = [sys.executable, str(root / 'scripts/assess_ai_application_comparison.py'), '--tool', str(tool), '--bundle', str(good),
       '--comparison-sha256', sha(good / 'comparison.json'), '--policy-sha256', sha(good / 'policy.json'), '--output', str(work / 'assessment.json')]
assert subprocess.run(cli, capture_output=True, text=True, timeout=120).returncode == 0
cli[cli.index('--comparison-sha256') + 1] = '0' * 64
assert subprocess.run(cli, capture_output=True, text=True, timeout=120).returncode == 2
assert json.loads((work / 'assessment.json').read_text())['success'] is False
checks += 2
print(f'PASS {checks} comparison integrity, native replay, sampling, policy and regression cases; synthetic fixtures only')
