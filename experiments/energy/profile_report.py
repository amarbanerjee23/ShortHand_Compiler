#!/usr/bin/env python3
"""Replay diagnostic stage timings; never accept these as energy evidence."""
import argparse
import hashlib
import json
import math
import pathlib

STAGES = ('preprocessing_ns', 'prepared_call_ns', 'output_validation_ns', 'postprocessing_ns')
LABELS = ('Input validation and preprocessing', 'Prepared backend call (including telemetry)',
          'Output validation', 'Score ownership and top-k')


def positive_integer(value):
    return type(value) is int and value > 0


def validate(report):
    if report.get('schema') != 'shorthand.ai.application.profile.v1':
        raise ValueError('not a diagnostic profile')
    for key in ('success', 'diagnostic_only'):
        if report.get(key) is not True:
            raise ValueError('unsuccessful/non-diagnostic profile')
    for key in ('production_claim', 'comparative_energy_claim', 'official_certification_granted',
                'measured_energy_available', 'latency_claim_eligible'):
        if report.get(key) is not False:
            raise ValueError('profile cannot authorize claims: ' + key)
    for key in ('rows', 'repetitions', 'batch_size', 'threads'):
        if not positive_integer(report.get(key)):
            raise ValueError('invalid profile dimension: ' + key)
    for key in ('accuracy', 'minimum_accuracy'):
        value = report.get(key)
        if type(value) not in (int, float) or not math.isfinite(value) or not 0 <= value <= 1:
            raise ValueError('invalid quality metric')
    if report['accuracy'] < report['minimum_accuracy']:
        raise ValueError('profile failed quality threshold')
    trials = report.get('trials')
    if not isinstance(trials, list) or not 1 <= len(trials) <= 25:
        raise ValueError('missing/bounded trial data')
    completed = report['rows'] * report['repetitions']
    batches = ((report['rows'] + report['batch_size'] - 1) // report['batch_size']) * report['repetitions']
    for trial in trials:
        for key in (*STAGES, 'total_ns', 'completed', 'batches'):
            if type(trial.get(key)) is not int or trial[key] < 0:
                raise ValueError('invalid integer timing/count: ' + key)
        if trial['completed'] != completed or trial['batches'] != batches:
            raise ValueError('incomplete profile work')
        if trial['total_ns'] <= 0 or sum(trial[key] for key in STAGES) != trial['total_ns']:
            raise ValueError('timing partition mismatch')
        outer = trial.get('observer_elapsed_ms')
        if type(outer) not in (int, float) or not math.isfinite(outer) or outer < trial['total_ns'] / 1e6:
            raise ValueError('invalid enclosing observer time')
    return {key: sum(trial[key] for trial in trials) for key in (*STAGES, 'total_ns', 'completed')}


def markdown(report, digest):
    totals = validate(report)
    lines = ['# Runtime diagnostic profile', '',
             '**Instrumented timing only: no energy or uninstrumented latency claim.**', '',
             'This partitions calls to ClassificationApplication. Dataset slicing, reference checks and',
             'trial aggregation are outside the partition. Clock overhead is included.', '',
             'The prepared call includes backend validation, ONNX execution, output copying and telemetry.',
             'Its share does not isolate ONNX kernels or establish which internal operation is expensive.', '',
             f"Batch: {report['batch_size']}; threads: {report['threads']}; rows: {report['rows']}; "
             f"repetitions: {report['repetitions']}; trials: {len(report['trials'])}.", '',
             '| Stage | Aggregate milliseconds | Nanoseconds / completed image | Share of instrumented time |',
             '| --- | ---: | ---: | ---: |']
    for key, label in zip(STAGES, LABELS):
        lines.append(f'| {label} | {totals[key] / 1e6:.6f} | '
                     f'{totals[key] / totals["completed"]:.3f} | {100 * totals[key] / totals["total_ns"]:.2f}% |')
    lines.extend(['', 'These are descriptive aggregates from one capture, not independent-session confidence intervals.',
                  'Do not compare these instrumented numbers to historical uninstrumented benchmark timings.', '',
                  f'Raw input SHA-256: `{digest}`. All trial values are retained in `profile.json`.', ''])
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', required=True, type=pathlib.Path)
    parser.add_argument('--input-sha256', required=True)
    parser.add_argument('--output', required=True, type=pathlib.Path)
    args = parser.parse_args()
    if args.input.stat().st_size > 32 * 1024 * 1024:
        raise ValueError('profile too large')
    data = args.input.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != args.input_sha256:
        raise ValueError('profile digest mismatch')
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError('duplicate JSON field')
            result[key] = value
        return result
    report = json.loads(data, object_pairs_hook=pairs)
    rendered = markdown(report, digest)
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / 'profile.json').write_bytes(data)
    (args.output / 'PROFILE.md').write_text(rendered)


if __name__ == '__main__':
    main()
