# Public Release Readiness Gate

public_release_readiness_version: 2026-09-12-pr94
current_maturity: controlled_beta
production_claim: false
release_candidate_target: PR96

A GitHub PR94 candidate requires all mandatory commands to pass from a clean checkout:

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
```

This command set qualifies the current PR candidate only. Public enterprise release requires the roadmap PR96 zero-skip scoped aggregate and applicable PR97-PR102 audit closeout gates, every production blocker closed, both stable CI contexts green on the final head, and the protected release exercise completed.

Historical readiness marker: public_release_readiness_version: 2026-09-01-pr88.

## Mandatory Declared-Scope Checks

ONNX Runtime CPU live numerical execution and the ephemeral Kubernetes gate are mandatory on the inherited Linux x64 CI lane. An unavailable `ONNXRUNTIME_ROOT`, container runtime or cluster fails that declared-scope lane. Mandatory checks may not be converted to warnings, `continue-on-error`, unconditional skips or false-success fallback.

## Skipped Optional Checks

Experimental paths outside `linux-x64-cpu-v1` may be unavailable without expanding the production claim. This can include `LIBTORCH_ROOT`, TensorRT/OpenVINO/llama.cpp SDKs, accelerator-only `RAPL/NVML` telemetry, and platform-specific measurement tools. Their absence must be reported as unavailable, never as executed-and-verified. They must become mandatory with retained evidence before the production support set or an energy claim is expanded to depend on them.

## Claims Policy

Do not make unsupported production, certification, external-publication, absolute defect-freedom, inherent-greenness, carbon-neutrality or guaranteed-savings claims. C3-ECO outputs remain candidate evidence only. Electricity-cost statements require measured kWh reduction, disclosed tariff, boundary and uncertainty. The scoped reliability wording remains: no known bugs under the full validation suite, after that suite passes.

GitHub PR94 implements original roadmap PR93 in the Linux x64/LLVM18 scope: verified SemanticIR, source and SDK lowering, bounded composite values, checked real ONNX runtime calls and optimization-preserved evidence. See [the lowering contract](mlir_lowering.md). TST024 is implemented within this scope. Merged GitHub PR93 is the separate gap assessment; roadmap PR94-PR102 remain future implementation IDs. Ten increments remain including this candidate, nine after it, subject to complete exit evidence and external operational blockers.
