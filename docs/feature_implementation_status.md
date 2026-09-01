# Feature Implementation Status

feature_status_version: 2026-09-01-pr88
language_version: beta-0.7
current_maturity: controlled_beta
production_claim: false
current_github_pr: 88
current_roadmap_scope: typed_c3eco_certification_profile

## Goal

ShortHand is intended to become a production-grade compiled AI language that lets engineers build and deploy AI software without Python, with predictable semantics, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

## Current baseline

Roadmap PR69 through PR79 are merged. Roadmap PR74 was implemented and merged as GitHub PR75, roadmap PR75 as GitHub PR76, and roadmap PR76 through PR81 as GitHub PR77 through PR82. GitHub PR83 implemented production truth and C3-ECO traceability, GitHub PR84 implemented beta-0.4 type/memory, GitHub PR85 implemented beta-0.5 functions/control flow, GitHub PR86 implemented the bounded beta-0.6 enterprise schema/packages/core FFI, and GitHub PR87 implemented the process-scoped concurrent serving and operational runtime. GitHub PR88 now implements the beta-0.7 typed C3-ECO certification-preparation profile.

The compiler test audit records **27 implemented, 3 partial and 3 open** areas for the PR88 candidate. ShortHand remains a controlled beta because measurement, carbon accounting, scoring, auditor lifecycle, composite execution lowering, public authenticated service ingress, MLIR, representative workloads, performance, measured-energy and protected-release blockers remain.

## Production truth authority

The active state is machine-readable in `docs/production_truth.tsv`. C3-ECO readiness is tracked in `docs/c3eco_traceability.tsv` across mandatory gates G1-G14, scoring domains A-K and the applicable S9/S12 software classes. `scripts/check_production_truth.sh` fails when active documents contradict the source, required rows are missing, evidence paths are invalid or an implemented certification row lacks verification evidence.

The current certification profile treats the supplied C3-ECO draft v0.6 as the normative candidate and the 2026-07-18 v0.7 all-inclusive and eligibility documents as an overlay. These are consultation drafts. The compiler produces candidate evidence only and does not grant certification.

## Language and compiler status

| Area | Status | Evidence / boundary |
| --- | --- | --- |
| Base grammar and module extension matrices | Implemented | beta-0.2 grammar plus beta-0.3 module conformance gates. |
| Module/import/package syntax and AST scaffold | Implemented | module AST and source identity contract. |
| Deterministic module resolver and multi-file codegen | Implemented | manifest/lock graph, confinement, imported execution and native linking. |
| Cross-mode semantic equivalence | Implemented for defined beta-0.5 typed/control-flow contract | interpreter, `lli` and native differential execution. |
| Production type and memory model | Implemented for `shorthand.type_memory.v1` | Exact float/string/typed-array execution plus composite descriptors, checked storage and guarded ownership states; composite source syntax remains bounded. |
| Functions, structured control flow and deterministic errors | Implemented for `shorthand.control_flow.v1` | Expression calls, recursion, lexical locals/cleanup, exact returns and same-block label resolution under the beta-0.5 differential gate. |
| Enterprise schemas and ownership plans | Implemented for `shorthand.enterprise_language.v1` | Namespaced record/enum/slice/option/result schemas and explicit move/borrow validation; composite execution is not claimed. |
| Offline packages, core library and safe FFI | Implemented for package/lock v2 and core ABI 1.0.0 | Exact versions, SHA-256, license allowlist, SPDX output, static/shared C/C++ consumers and frozen core exports. |
| Source-aware diagnostics | Implemented for current coded matrix | LSP publication uses the same compiler diagnostic oracle. |
| Full sanitizer coverage | Implemented for current baseline | ASan/LSan/UBSan compiler/runtime coverage. |
| Continuous fuzzing | Implemented for current compiler stages | parser/module/semantic/lowering libFuzzer plus scheduled extension. |
| Concurrency and race detection | Implemented for current runtime baseline | functional thread-safety plus mandatory TSan. |
| Compiled-code metadata/runtime lowering | Partial | generated MLIR and production lowering remain PR92-PR93. |
| Cross-platform portability | Implemented for PR74 tiers | GCC12/14, Clang16/18, Linux x64/arm64, macOS arm64, Windows x64. |
| Cross-platform reproducibility | Implemented | independent clean builds and checksum/tamper gate. |
| Runtime ABI compatibility | Implemented v1 | frozen 25-symbol ABI consumer plus multi-platform installed consumers. |
| Packaging and installed consumers | Implemented | install/reinstall/uninstall lifecycle on qualified platforms. |
| Concurrent serving and operational runtime | Implemented for `shorthand.serving.runtime.v1` | Bounded concurrency/admission, cooperative cancellation, deadlines, process-scoped tenant isolation, health, metrics, graceful drain and installed worker evidence; public ingress/authentication/TLS are not claimed. |
| Typed C3-ECO certification profile | Implemented for `shorthand.c3eco.profile.v2` | Native field types, deterministic profile links, identity, functional unit, boundary/materiality, lifecycle, safeguard and validity checks plus fail-closed v1 migration output. |
| Production truth and C3-ECO traceability | Implemented for `shorthand.production.truth.v1` | Active maturity/roadmap authority plus G1-G14, A-K and S9/S12 evidence ownership. |

Historical compatibility term: Module/import/package model.
Historical compatibility gate: language versioning and conformance policy gate.

## Runtime, backend and hardware status

| Area | Status | Evidence / boundary |
| --- | --- | --- |
| Real ONNX Runtime CPU backend execution | Implemented for `linux-x64-cpu-v1` | Pinned ONNX Runtime SDK, real identity-model execution and numerical output `42`; representative production AI workload qualification remains PR94. |
| Full backend compatibility | Implemented for declared v1 production support set | Only `onnxruntime_cpu` + CPU is production-supported. Other backend/device pairs remain experimental or unavailable and are not advertised as production support. |
| Runtime observability implementation | Implemented for process-scoped serving v1 | Versioned health JSON and low-cardinality Prometheus metrics are integrated with readiness, saturation and drain; public network exposure is not implied. |
| CPU/GPU/TPU/NPU routing | Implemented for qualification-aware v1 policy | Inventory remains available for all device classes. Production routing rejects unqualified accelerator pairs by default. |

## Security, release, deployment and tooling status

| Area | Status | Boundary |
| --- | --- | --- |
| Automated SBOM | Implemented candidate and artifact baseline | SPDX 2.3 source evidence plus per-release-artifact bundle evidence. |
| Secret and claim scanning | Implemented | repository secret baseline plus mandatory Trivy secret scan and release claim safety. |
| Signed releases | Partial | Immutable tag policy, OIDC artifact/SBOM attestations, cryptographic verification, draft verification and rollback exist; real signed publication still requires protected-environment execution. |
| Protected publication | Partial | Workflow fails closed unless required reviewers, prevent-self-review and exact `v*` deployment policy are live. |
| External vulnerability gate | Implemented for current repository/dependency contract | mandatory CodeQL, Trivy, dependency delta review, license policy and expiring exceptions. |
| Container and Kubernetes hardening | Implemented for the PR87 serving-worker deployment contract | Native amd64/arm64 hardened images plus restricted live Kind qualification, worker health probes and pre-stop drain. |
| Formatter and linter | Implemented for `shorthand.tooling.format_lint.v1` | deterministic trivia-only formatting, machine diagnostics and safe explicit-output fixes. |
| Syntax highlighting and LSP | Implemented for `shorthand.tooling.lsp.v1` | scanner-aligned grammar; native bounded JSON-RPC; compiler diagnostics; UTF-16; completion, hover, definitions, symbols, module navigation and cancellation. |

## Green AI and compiler-backend status

| Area | Status | Boundary |
| --- | --- | --- |
| Green AI contract/evidence syntax | Implemented for current beta syntax, first-class C3-ECO declarations and typed profile v2 | GitHub PR82 adds `shorthand.c3eco.language.v1`; GitHub PR88 adds `shorthand.c3eco.profile.v2`; measurement and scoring remain separate. |
| C3-ECO scoring/auditor evidence | Partial | First-class language blocks and typed profiles are implemented; measurement, scoring and auditor lineage remain PR89-PR91. |
| MLIR dialect scaffold | Partial | the hand-authored foundation exists; generated dialect and production lowering remain PR92-PR93. |
| Measured ShortHand versus Python energy evidence | Open | PR95. No lower-energy claim is made by current CI. |
| Zero-skip production RC gate | Open | PR96. |

## C3-ECO language PR82 boundary

c3eco_language_contract: shorthand.c3eco.language.v1

GitHub PR82 implemented roadmap PR81 with ten first-class declaration kinds: `certification`, `functional_unit`, `workload`, `boundary`, `measurement_plan`, `ai_lifecycle`, `rag_pipeline`, `token_budget`, `model_routing` and `guardrails`. The parser, AST, semantic analyzer, IR metadata and evidence emitter carry the same structured declaration data. Required fields fail closed with `SHD5102`, duplicate declarations with `SHD5101`, invalid fields with `SHD5103`, and attempted self-certification claims with `SHD5104`.

These declarations are evidence inputs only. They cannot grant certification, create a certificate identifier or mark a candidate as officially certified; generated C3-ECO candidate evidence keeps `official_certification_granted:false`. Measured scoring and external authority/auditor handoff remain later roadmap work.

## Production backend and hardware qualification PR81 boundary

kubernetes_qualification_gate: verified

Kubernetes qualification gate: Verified. The mandatory PR81 path preserves live Kind/cluster qualification through `scripts/check_kubernetes_cluster.sh` and `tests/integration/test_production_backend_hardware_qualification.sh`; missing-cluster, skipped-execution, fallback and unqualified production routing remain fail-closed.

GitHub PR81 implements roadmap PR80 through `shorthand.backend_hardware_qualification.v1`. The production support scope is deliberately versioned as `linux-x64-cpu-v1`. `onnxruntime_cpu` on CPU is the only production-supported backend/device pair in this contract.

The mandatory qualification path downloads ONNX Runtime 1.20.1 from its fixed release asset, verifies the pinned SHA-256, then runs the real C++ runtime bridge against the checked-in identity ONNX model. Input `42` must produce output `42`, the runtime must report `onnxruntime_cpu`, and fallback, not-executed or skip evidence fails the gate.

Hardware discovery continues to inventory CPU, GPU, TPU and NPU. A detected accelerator, installed SDK or compatible policy row is not enough for production routing. Unqualified GPU/NPU/TPU routes fail closed with `backend_device_not_production_qualified`. `SHORTHAND_ALLOW_UNQUALIFIED_BACKEND_HARDWARE=1` is an explicit development override and its evidence remains `production_qualified:false`.

TST022 is implemented for this declared v1 support set after the final GitHub PR81 head passed both stable CI contexts. Adding a production GPU, TPU, NPU, backend or platform later requires real device-backed numerical evidence first.

## Syntax highlighting and LSP PR80 boundary

GitHub PR80 implemented roadmap PR79 through `shorthand.tooling.lsp.v1`. `shorthand_lsp` is native C++17, bounded, compiler-backed and installed by CMake. The contract does not claim rename/refactor, semantic tokens, workspace-wide indexing or debugger support.

## Formatter and linter PR79 boundary

GitHub PR79 implemented roadmap PR78 through native `shorthand_tool` and contract `shorthand.tooling.format_lint.v1`.

## Signed release PR76 boundary

GitHub PR76 introduced `.github/workflows/release.yml`, immutable release tag/master-lineage policy, protected-environment preflight, OIDC attestations and draft-release rollback. TST017 remains **partial**, not implemented: workflow source code is not cryptographic execution evidence. The blocker closes only after repository administration configures `production-release` and an actual tag release produces attestations that `gh attestation verify` accepts.

## External security PR77 boundary

GitHub PR77 implemented roadmap PR76 with mandatory CodeQL C++ `security-extended`, Trivy repository scans, a fail-closed dependency delta gate, redistribution license policy, immutable action pins and 90-day maximum security exceptions.

## Container and Kubernetes PR78 boundary

GitHub PR78 implemented roadmap PR77 through a multi-stage production image and Restricted Pod Security workload. GitHub PR87 extends that deployment with the real bounded serving worker, versioned startup/liveness/readiness probes, self-test and pre-stop drain evidence. No public service or ingress claim is introduced.

## Concurrent serving PR87 boundary

serving_runtime_contract: shorthand.serving.runtime.v1

GitHub PR87 implements bounded process-scoped scheduling with nonblocking overload rejection, enforced deadlines, cooperative cancellation, tenant isolation, bounded request/response/result retention, low-cardinality metrics and graceful drain. The same installable worker is exercised by native tests, installed consumers, sanitizers, TSan, Docker and Kubernetes lifecycle checks.

The contract deliberately does not create a public listener. Authentication, authorization, TLS, external rate limiting and hard termination of handlers that ignore the cancellation token remain host/process-boundary responsibilities. These limits keep `production_claim: false` honest.

## Typed C3-ECO profile PR88 boundary

c3eco_profile_contract: shorthand.c3eco.profile.v2

GitHub PR88 adds a contextual `certification_profile` block and native string, identifier, integer, decimal and boolean values for the declarations it links. The compiler validates identity, useful-work units, workload bounds, materiality, lifecycle responsibility, safeguard floors and validity dates with `SHD5201` through `SHD5208`. Legacy v1 declarations remain valid and are explicitly marked for migration review.

Profile conformance is not certification. Measurement, carbon/cost accounting, scoring, permitted level claims, signed auditor lineage and external authority approval remain PR89 through PR91. Candidate and migration outputs retain `official_certification_granted:false`.

## Production blockers

1. C3-ECO measurement, scoring and auditor lifecycle (PR89-PR91).
2. Generated MLIR dialect, composite execution integration and production lowering (PR92-PR93).
3. Representative production AI workload qualification (PR94).
4. Measured performance/energy comparison and zero-skip production RC gate (PR95-PR96).
5. Configure and exercise the protected signed-release environment for TST017 after the implementation backlog closes.

GPU/TPU/NPU support is not a blocker for the declared `linux-x64-cpu-v1` production backend contract because those device classes are explicitly not production-supported in v1. Any future support expansion becomes a production blocker for that expanded version until its live device-backed tests pass.

## Historical audit anchors

These exact strings are retained as audit history only and are not active state:

- feature_status_version: 2026-08-18-pr80
- 20 implemented, 4 partial and 3 open
- GitHub PR80 now implements roadmap PR79
- Syntax highlighting and LSP | Implemented for `shorthand.tooling.lsp.v1` candidate
- live production qualification remains PR80
- feature_status_version: 2026-08-18-pr79
- 19 implemented, 4 partial and 4 open
- GitHub PR79 now implements roadmap PR78
- Formatter and linter | Implemented
- Syntax highlighting and LSP | Open
- feature_status_version: 2026-08-12-pr78
- 18 implemented, 4 partial and 5 open
- GitHub PR78 now implements roadmap PR77
- Container and Kubernetes hardening | Implemented
- feature_status_version: 2026-08-12-pr77
- 17 implemented, 4 partial and 6 open
- External vulnerability gate | Implemented
- feature_status_version: 2026-08-12-pr76
- 16 implemented, 5 partial and 6 open
- Signed releases | Partial
- feature_status_version: 2026-08-11-pr73
- 12 implemented, 6 partial and 9 open

## Review rule

Any PR that changes syntax, semantic meaning, runtime behavior, editor behavior, release/evidence output, pipeline behavior, test coverage or production claims must update this tracker and `tests/coverage/compiler_test_coverage_matrix.tsv`, while retaining historical anchors required for milestone auditability.
