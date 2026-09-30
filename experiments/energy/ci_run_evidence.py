#!/usr/bin/env python3
"""Always-retained CI evidence index; absent evidence cannot silently pass the gate."""
import argparse
import datetime
import hashlib
import json
import os
import pathlib
import subprocess

REQUIRED = {
    'compiled_session_reuse_and_invalidation': 'cache-tests.txt',
    'resident_workspace_correctness': 'workspace-tests.txt',
    'real_digit_application_correctness': 'application-tests.txt',
    'compiled_fp32_observations': 'generated-infer/report.json',
    'same_runner_head_base_observations': 'pr-runtime-delta/summary.json',
    'resident_native_and_python_controls': 'resident-baselines/summary.json',
    'energy_capabilities': 'ci-energy-probe.json',
    'resident_component_energy_or_unavailable': 'ci-energy/summary.json',
}
SCHEMAS = {
    'compiled_fp32_observations': 'shorthand.generated_infer.report.v1',
    'same_runner_head_base_observations': 'shorthand.energy.pr_runtime_delta.v1',
    'resident_native_and_python_controls': 'shorthand.energy.resident_baselines.v1',
    'energy_capabilities': 'shorthand.energy.ci_evidence.v1',
    'resident_component_energy_or_unavailable': 'shorthand.energy.ci_component_comparison.v1',
}
PASS_MARKERS = {
    'compiled_session_reuse_and_invalidation': 'PASS real ONNX cache reuse',
    'resident_workspace_correctness': 'PASS host classification:',
    'real_digit_application_correctness': 'live_onnx=1',
}


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def initialize(out):
    out.mkdir(parents=True, exist_ok=True)
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
    metadata = dict(schema='shorthand.ci.run_evidence.v1', revision=revision,
                    run_id=os.environ.get('GITHUB_RUN_ID'), attempt=os.environ.get('GITHUB_RUN_ATTEMPT'),
                    event=os.environ.get('GITHUB_EVENT_NAME', 'local'),
                    run_url=f"{os.environ.get('GITHUB_SERVER_URL', 'https://github.com')}/{os.environ.get('GITHUB_REPOSITORY', '')}/actions/runs/{os.environ.get('GITHUB_RUN_ID', '')}",
                    started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                    status='incomplete', latency_reduction_claim_authorized=False,
                    energy_reduction_claim_authorized=False, power_reduction_claim_authorized=False,
                    required_evidence=REQUIRED)
    write(out / 'run-evidence.json', metadata)
    return metadata


def finalize(out, job_status):
    path = out / 'run-evidence.json'
    metadata = json.loads(path.read_text()) if path.exists() else initialize(out)
    evidence, missing = {}, []
    for claim, relative in REQUIRED.items():
        artifact = out / relative
        present = artifact.is_file() and artifact.stat().st_size > 0
        if present and artifact.suffix == '.json':
            try:
                value = json.loads(artifact.read_text())
                present = (isinstance(value, dict) and value.get('schema') == SCHEMAS[claim]
                           and value.get('success') is not False)
                if 'energy' not in claim:
                    present = present and value.get('success') is True
                if value.get('available') is False and 'energy' not in claim:
                    present = False
            except (ValueError, OSError):
                present = False
        if present and artifact.suffix == '.txt':
            present = PASS_MARKERS[claim] in artifact.read_text()
        evidence[claim] = dict(path=relative, status='captured' if present else 'missing_or_failed')
        if not present:
            missing.append(claim)
    # Meter absence is an explicit observation, never a made-up zero or a failed
    # correctness test. Failed/missing execution evidence DOES fail the CI gate.
    metadata.update(status='complete' if not missing and job_status == 'success' else 'incomplete',
                    job_status=job_status, completed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                    evidence=evidence, missing=missing,
                    raw_artifact_retention_days=90,
                    limitation='shared-runner timings are descriptive; no reduction claim is authorized; '
                               'manual cancellation or runner loss can prevent finalization')
    metadata['hashes'] = {str(p.relative_to(out)): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in sorted(out.rglob('*')) if p.is_file() and p.name not in ('run-evidence.json', 'EVIDENCE.md')}
    write(path, metadata)
    lines = ['# Per-run latency and energy evidence', '',
             f"Revision: `{metadata['revision']}`. Collection: **{metadata['status']}**.", '',
             '| Evidence | Status | Artifact |', '|---|---|---|']
    for claim, record in evidence.items():
        lines.append(f"| {claim} | {record['status']} | `{record['path']}` |")
    lines += ['', 'No latency, energy or power reduction claim is authorized by this index.',
              'Joules require an available meter. Estimates remain estimates; unavailable means null.',
              'Raw data, source, hashes, commands and reports are retained for 90 days. Archive them before expiry for longer-lived claims.', '']
    (out / 'EVIDENCE.md').write_text('\n'.join(lines))
    return metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--initialize', action='store_true')
    parser.add_argument('--job-status', choices=['success', 'failure', 'cancelled'], default='success')
    args = parser.parse_args()
    if args.initialize:
        initialize(args.output)
        return 0
    result = finalize(args.output, args.job_status)
    return 0 if result['status'] == 'complete' else 1


if __name__ == '__main__':
    raise SystemExit(main())
