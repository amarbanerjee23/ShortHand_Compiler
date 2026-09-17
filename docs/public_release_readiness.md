# Public Release Readiness Gate

public_release_readiness_version: 2026-09-17-pr101
current_maturity: controlled_beta
production_claim: false
release_candidate_target: PR101

A GitHub PR101 candidate requires all mandatory commands to pass from a clean checkout, followed by the `shorthand.enterprise.pilot_rc.v1` lifecycle and blocker-aggregation gate:

```bash
bash setup_build_infra.sh
source ./shorthand_env.sh
bash scripts/validate_language.sh --strict
bash scripts/check_production_truth.sh
bash tests/governance/test_production_truth_negative.sh
bash scripts/check_production_type_memory_model.sh
bash scripts/check_functions_control_error_semantics.sh
bash scripts/check_enterprise_packages_stdlib_ffi.sh
bash scripts/check_concurrent_serving_runtime.sh
bash scripts/check_c3eco_certification_profile.sh
bash scripts/check_c3eco_measurement_workbook.sh
bash scripts/check_c3eco_assessment.sh
bash scripts/check_c3eco_auditor_bundle.sh
bash scripts/check_semantic_differential.sh
bash scripts/smoke_test.sh
bash scripts/check_mlir_dialect.sh
make -C Compiler_new_ws/Short_Hand/src test
make -C Compiler_new_ws/Short_Hand/src sanitize
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
bash tests/enterprise/test_production_rc_contract.sh
bash scripts/check_production_rc.sh build /tmp/shorthand-pr101-stage
```

This command set qualifies the current PR candidate only. PR99's aggregate must emit a blocked decision when retained evidence remains. Public enterprise release requires the roadmap PR96/PR99 zero-skip scoped aggregate and applicable PR100-PR102 audit closeout gates, every production blocker closed, both stable CI contexts green on the final head, and the protected release exercise completed.

Historical readiness marker: public_release_readiness_version: 2026-09-01-pr88.

## Mandatory Declared-Scope Checks

ONNX Runtime CPU live numerical execution and the ephemeral Kubernetes gate are mandatory on the inherited Linux x64 CI lane. An unavailable `ONNXRUNTIME_ROOT`, container runtime or cluster fails that declared-scope lane. Mandatory checks may not be converted to warnings, `continue-on-error`, unconditional skips or false-success fallback.

## Skipped Optional Checks

Experimental paths outside `linux-x64-cpu-v1` may be unavailable without expanding the production claim. This can include `LIBTORCH_ROOT`, TensorRT/OpenVINO/llama.cpp SDKs, accelerator-only `RAPL/NVML` telemetry, and platform-specific measurement tools. Their absence must be reported as unavailable, never as executed-and-verified. They must become mandatory with retained evidence before the production support set or an energy claim is expanded to depend on them.

## Claims Policy

Do not make unsupported production, certification, external-publication, absolute defect-freedom, inherent-greenness, carbon-neutrality or guaranteed-savings claims. C3-ECO outputs remain candidate evidence only. Electricity-cost statements require measured kWh reduction, disclosed tariff, boundary and uncertainty. The scoped reliability wording remains: no known bugs under the full validation suite, after that suite passes.

GitHub PR94 implements original roadmap PR93 in the Linux x64/LLVM18 scope: verified SemanticIR, source and SDK lowering, bounded composite values, checked real ONNX runtime calls and optimization-preserved evidence. See [the lowering contract](mlir_lowering.md). TST024 is implemented within this scope. Merged GitHub PR93 is the separate gap assessment; PR100 is merged; PR101 is the active independent-pilot candidate and PR102 is the one remaining implementation batch afterward. Physical observations, full family baselines, independently reviewed organizational operations and the protected release exercise remain mandatory external evidence.

PR97 implements [measurement replay and regression controls](ai_comparison_measurement.md). PR99 implements the enterprise pilot/RC aggregate while preserving the explicit CPU-only scope and retained blockers. The mandatory native gate adds synthetic replay/negative cases, and the real locked Python gate assesses its execution bundle. Calibrated physical observations, broader workload baselines and enterprise GA evidence remain open.

Historical readiness marker: public_release_readiness_version: 2026-09-15-pr97; release_candidate_target: PR96.

PR101 implements [independent reproduction and draft-standard pilot verification](c3eco_independent_pilot.md) over signed PR91 bundles. Candidate consistency receipts do not authenticate physical measurements, legal independence, actual storage retention or certification. G8/S9/S12 and TST017/TST025/TST026 remain partial; PR102 is the remaining planned implementation batch after PR101.
