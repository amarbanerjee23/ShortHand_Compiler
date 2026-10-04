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
    'compiled_cache_boundary_and_lifecycle_stress': 'cache-tests.txt',
    'compiled_descriptor_identity_and_policy': 'cache-tests.txt',
    'resident_workspace_correctness': 'workspace-tests.txt',
    'resident_workspace_soak': 'workspace-tests.txt',
    'prepared_cache_and_workspace_sanitizers': 'prepared-sanitizers.txt',
    'evidence_regression_tests': 'evidence-tests.txt',
    'benchmark_markdown': 'BENCHMARKS.md',
    'real_digit_application_correctness': 'application-tests.txt',
    'compiled_fp32_observations': 'generated-infer/report.json',
    'compiled_phase_attribution': 'generated-infer/phases.json',
    'compiled_phase_execution_tests': 'phase-tests.txt',
    'compiled_phase_production_isolation': 'phase-tests.txt',
    'same_runner_head_base_observations': 'pr-runtime-delta/summary.json',
    'resident_native_and_python_controls': 'resident-baselines/summary.json',
    'energy_capabilities': 'ci-energy-probe.json',
    'resident_component_energy_or_unavailable': 'ci-energy/summary.json',
}
SCHEMAS = {
    'compiled_fp32_observations': 'shorthand.generated_infer.report.v2',
    'compiled_phase_attribution': 'shorthand.generated_infer.phases.v1',
    'same_runner_head_base_observations': 'shorthand.energy.pr_runtime_delta.v1',
    'resident_native_and_python_controls': 'shorthand.energy.resident_baselines.v1',
    'energy_capabilities': 'shorthand.energy.ci_evidence.v1',
    'resident_component_energy_or_unavailable': 'shorthand.energy.ci_component_comparison.v1',
}
PASS_MARKERS = {
    'compiled_session_reuse_and_invalidation': 'PASS real ONNX cache reuse',
    'compiled_cache_boundary_and_lifecycle_stress': 'PASS extended ONNX cache boundaries and lifecycle stress',
    'compiled_descriptor_identity_and_policy': 'PASS cached descriptor alternating names, changed input and warm policy refresh',
    'resident_workspace_correctness': 'PASS host classification:',
    'resident_workspace_soak': 'PASS workspace soak:',
    'prepared_cache_and_workspace_sanitizers': 'PASS prepared cache and workspace ASan/UBSan',
    'evidence_regression_tests': 'PASS evidence regression suites',
    'benchmark_markdown': '# Testing and benchmark results',
    'real_digit_application_correctness': 'live_onnx=1',
    'compiled_phase_execution_tests': 'PASS compiled phase profiling:',
    'compiled_phase_production_isolation': 'PASS phase profile isolation: production archive has no diagnostic symbols',
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
        present = (artifact.is_file() and artifact.stat().st_size > 0
                   and not any((out / pathlib.Path(*pathlib.Path(relative).parts[:i])).is_symlink()
                               for i in range(1, len(pathlib.Path(relative).parts) + 1)))
        if present and artifact.suffix == '.json':
            try:
                value = json.loads(artifact.read_text())
                present = (isinstance(value, dict) and value.get('schema') == SCHEMAS[claim]
                           and value.get('success') is not False)
                if 'energy' not in claim:
                    present = present and value.get('success') is True
                if value.get('available') is False and 'energy' not in claim:
                    present = False
                if any(v is not False for k, v in value.items()
                       if k.endswith(('_claim', '_claim_authorized', '_claim_eligible')) or k == 'claim_authorized'):
                    present = False
                if claim == 'resident_component_energy_or_unavailable':
                    present = present and type(value.get('available')) is bool
                    if value.get('available') is False:
                        present = (present and value.get('evidence_class') is None
                                   and value.get('component_energy_measured') is False
                                   and bool(value.get('reason')))
            except (ValueError, OSError):
                present = False
        if present and artifact.suffix in ('.txt', '.md'):
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
    for kind in ('latency', 'energy', 'power'):
        metadata[kind + '_reduction_claim_authorized'] = False
    metadata['hashes'] = {str(p.relative_to(out)): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in sorted(out.rglob('*')) if p.is_file() and not p.is_symlink()
                          and p.resolve().is_relative_to(out.resolve())
                          and p.name not in ('run-evidence.json', 'EVIDENCE.md')}
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
