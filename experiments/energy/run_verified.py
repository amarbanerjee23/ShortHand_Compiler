#!/usr/bin/env python3
"""One declared execution-only campaign; failed stages remain in the evidence."""
import argparse
import datetime
import os
import pathlib
import platform
import subprocess
import sys
import traceback

import campaign
import runtime_state_of_practice
import state_of_practice


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--clang', required=True)
    parser.add_argument('--tool', required=True)
    parser.add_argument('--onnxruntime-root', required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    index_path = out / 'run.json'
    if index_path.exists():
        raise ValueError('refusing to overwrite an existing capture')
    index = dict(schema='shorthand.energy.verified-run.v1',
                 mode='execution_only', started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                 revision=subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
                 run_id=os.environ.get('GITHUB_RUN_ID'), run_attempt=os.environ.get('GITHUB_RUN_ATTEMPT'),
                 platform=platform.platform(), python=sys.version, affinity=sorted(os.sched_getaffinity(0)),
                 stages=[], design=dict(source_pairs=30, source_repetitions=[10, 100],
                                        runtime_pairs=10, runtime_repetitions=10, runtime_trials=3),
                 unavailable_baselines={'rust-candle': 'repository supplies a runner contract, no implementation',
                                        'mojo-max': 'repository supplies a runner contract, no implementation'},
                 energy_savings_percent=None, **campaign.CLAIMS)
    campaign.write(index_path, index)

    def stage(name, function):
        row = dict(name=name, success=False)
        index['stages'].append(row)
        campaign.write(index_path, index)
        print('START ' + name, flush=True)
        try:
            function()
            row['success'] = True
        except Exception as error:
            row['error'] = str(error)
            (out / (name + '-failure.txt')).write_text(traceback.format_exc())
            print('FAIL ' + name + ': ' + str(error), flush=True)
        campaign.write(index_path, index)
        print('END ' + name + ': ' + str(row['success']), flush=True)

    def source(name, profile, repetitions):
        plan_dir = out / (name + '-plan')
        state_of_practice.prepare(argparse.Namespace(
            output=plan_dir, mode='execution_only', profile=profile, repetitions=repetitions,
            pairs=30, instrument=None, meter_csv=None, pytorch_python=sys.executable,
            rust_command=None, rust_version=None, mojo_command=None, mojo_version=None))
        plan = plan_dir / 'plan.json'
        state_of_practice.run(argparse.Namespace(
            plan=plan, plan_sha256=campaign.sha(plan), compiler=args.compiler, clang=args.clang,
            tool=args.tool, output=out / name))

    stage('source-r10', lambda: source('source-r10', 'torch', 10))
    # Keep the source matrix rectangular: every implemented baseline is run at
    # both repetition counts.  The previous core-only r100 capture silently
    # omitted PyTorch at the higher workload, which made startup amortization
    # comparisons incomplete.
    stage('source-r100', lambda: source('source-r100', 'torch', 100))

    plan_dir = out / 'runtime-plan'

    def prepare_runtime():
        campaign.prepare(argparse.Namespace(
            output=plan_dir, source_repetitions=10, runtime_repetitions=10,
            source_pairs=30, runtime_pairs=10, compile_repetitions=3, source_baseline='numpy',
            mode='execution_only', instrument=None, meter_csv=None))

    stage('runtime-plan', prepare_runtime)
    plan = plan_dir / 'plan.json'

    def python_runtime():
        campaign.run(argparse.Namespace(plan=plan, plan_sha256=campaign.sha(plan),
            compiler=args.compiler, clang=args.clang, tool=args.tool, output=out / 'python-runtime'))

    def cpp_runtime():
        runtime_state_of_practice.run(argparse.Namespace(plan=plan, plan_sha256=campaign.sha(plan),
            clang=args.clang, tool=args.tool, onnxruntime_root=args.onnxruntime_root,
            output=out / 'cpp-runtime'))

    stage('python-runtime', python_runtime)
    stage('cpp-runtime', cpp_runtime)
    index['completed_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    index['success'] = all(s['success'] for s in index['stages'])
    campaign.write(index_path, index)
    return 0 if index['success'] else 1


if __name__ == '__main__':
    sys.exit(main())
