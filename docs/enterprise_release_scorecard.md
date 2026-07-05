# Enterprise Release Readiness Scorecard

This scorecard is the control document for deciding whether ShortHand can move from internal engineering review to an enterprise release candidate. A release is not enterprise-ready unless every mandatory item marked `MUST` is satisfied with retained evidence.

## Readiness state labels

| Label | Meaning | Public wording |
| --- | --- | --- |
| ER0 | Internal engineering review | Research artifact / internal review only |
| ER1 | Compiler hardening candidate | Internal pilot candidate |
| ER2 | Real AI backend pilot | Controlled AI pilot candidate |
| ER3 | Enterprise beta candidate | Limited enterprise beta candidate |
| ER4 | Enterprise release candidate | Enterprise release candidate after all gates pass |

## Mandatory gates

| Gate | Requirement | Evidence | Status |
| --- | --- | --- | --- |
| G1 | Clean checkout build passes | CI run link and local reproduction command | Open |
| G2 | Strict language validation passes | `bash scripts/validate_language.sh --strict` | Open |
| G3 | Smoke tests pass | `bash scripts/smoke_test.sh` | Open |
| G4 | Full Makefile suite passes | `make -C Compiler_new_ws/Short_Hand/src test` | Open |
| G5 | Sanitizer suite passes | `make -C Compiler_new_ws/Short_Hand/src sanitize` | Open |
| G6 | CMake build and CTest pass | `cmake --build build` and `ctest --test-dir build --output-on-failure` | Open |
| G7 | Language specification is versioned | `docs/language_spec.md` with version and grammar coverage | Open |
| G8 | Compatibility and deprecation policy exists | policy document | Open |
| G9 | Positive and negative conformance tests cover all syntax | test matrix | Open |
| G10 | Parser and semantic diagnostics include file and line information | diagnostics test results | Open |
| G11 | At least one real AI backend executes in documented workflow | model execution evidence | Open |
| G12 | Fallback never claims executed inference | fallback tests and report evidence | Partially satisfied |
| G13 | Backend failure cases are covered | missing model, bad shape, unsupported runtime tests | Open |
| G14 | GreenAI evidence report is traceable | generated JSON and source manifest | Partially satisfied |
| G15 | Energy/carbon telemetry does not fabricate unavailable values | telemetry tests and report policy | Partially satisfied |
| G16 | Measurement plan exists for AI examples | measurement plan file | Open |
| G17 | SBOM generation is available | SBOM artifact | Open |
| G18 | Release artifacts are signed | signature artifact | Open |
| G19 | Dependency and secret scanning are configured | CI/security logs | Open |
| G20 | Security policy and disclosure process exist | security policy file | Open |
| G21 | Container image build exists | Dockerfile and image build CI | Open |
| G22 | Kubernetes/OpenShift deployment example exists | manifests and deployment docs | Open |
| G23 | Observability hooks exist | logs, metrics, health endpoints | Open |
| G24 | Governance and RFC process exists | RFC template and maintainer policy | Open |
| G25 | Unsupported public claims are blocked | claim scan gate | Partially satisfied |

## Current assessment

Current readiness state: ER0.

ShortHand should remain in internal engineering review until the following minimum improvements are complete:

1. PR-level CI must pass consistently.
2. A real ONNX Runtime CPU backend must execute a model.
3. Compiler diagnostics and conformance tests must cover the language surface.
4. Security, SBOM, signing, and release provenance must be added.
5. Deployment examples must exist for container and Kubernetes/OpenShift usage.
6. GreenAI evidence must include traceable runtime and measurement fields for at least one real AI workload.

## Promotion rules

### ER0 to ER1

Required:

- G1 to G10 pass.
- Known limitations remain documented.
- Unsupported claims remain blocked.

### ER1 to ER2

Required:

- ER1 complete.
- G11 to G16 pass.
- One real backend executes a model.
- Fallback and real execution are clearly separated in evidence.

### ER2 to ER3

Required:

- ER2 complete.
- G17 to G23 pass.
- Release artifacts have SBOM and signatures.
- Deployment example is reproducible.

### ER3 to ER4

Required:

- ER3 complete.
- G24 to G25 pass.
- Governance and long-term compatibility processes are active.
- Evidence bundle is retained.
- Maintainer sign-off is recorded.

## Required release evidence bundle

A release candidate evidence bundle must contain:

- CI run IDs.
- Toolchain versions.
- Build logs.
- Test reports.
- Sanitizer logs.
- CMake/CTest logs.
- AI backend execution report.
- GreenAI evidence JSON.
- Measurement plan.
- Carbon factor references.
- Security scan logs.
- SBOM.
- Signature/provenance files.
- Known limitations.
- Claims approval checklist.

## Merge policy for readiness PRs

A readiness PR may be merged only when it improves measurable gates without weakening safeguards. This scorecard PR itself should remain unmerged until reviewers agree that it correctly defines the readiness target and does not overstate current maturity.
