# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-06-pr68
PLAN_STATUS: active
BASELINE_AFTER_PR: 50
LAST_COMPLETED_PR: 68
BASELINE_LANGUAGE_VERSION: beta-0.2
TARGET: enterprise production usage ready language

Historical guard markers retained for old-task stability:

- production_readiness_plan_version: 2026-08-02-pr62
- LAST_COMPLETED_PR: 62
- production_readiness_plan_version: 2026-08-02-pr63
- LAST_COMPLETED_PR: 63
- production_readiness_plan_version: 2026-08-02-pr66
- LAST_COMPLETED_PR: 64
- LAST_COMPLETED_PR: 65
- LAST_COMPLETED_PR: 66
- production_readiness_plan_version: 2026-08-02-pr67
- LAST_COMPLETED_PR: 67

## Goal

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable language semantics, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security and auditable Green AI evidence.

Unsupported or unavailable execution paths must never report success. A skipped dependency, absent accelerator, source-pattern check or transport acceptance response is not production execution evidence.

## Current baseline

The current controlled-beta implementation has:

- a parser-accurate beta-0.2 grammar and executable conformance matrix,
- bounded malformed-input handling and parser resource ceilings,
- stable coded diagnostics and source ranges,
- selected AI semantic rejection cases,
- LLVM metadata and native runtime-linking paths,
- honest fallback and backend-unavailable behavior,
- hardware discovery and execution-ready routing policy,
- runtime ABI 1.0.0 with exactly 25 public symbols,
- serialized public runtime calls and thread-local evidence snapshots,
- Linux packaging, Prometheus and OTLP adapters,
- candidate C3-ECO schemas and claim-safety evidence,
- ASan, LSan and UBSan parser smoke coverage.

ShortHand remains `controlled_beta` with `production_claim: false`.

## Test audit correction applied in PR #68

The PR67 roadmap estimated 12 remaining PRs. A full compiler and CI audit found six omitted production-critical workstreams:

1. governed compiler test coverage and a mandatory per-PR test contract,
2. interpreter, LLVM and native differential semantic correctness,
3. continuous fuzzing, full sanitizer coverage and race detection,
4. cross-platform toolchain qualification and reproducible builds,
5. real live-backend and device qualification,
6. measured ShortHand-versus-Python performance and energy evidence.

After PR #67, 18 production-readiness PRs are required.

PR68 establishes the test strategy and coverage matrix. After PR #68 is merged, 17 implementation PRs remain.

The revised path contains 35 PRs from PR51 through PR85. Green Linux CI alone cannot prove enterprise language correctness, portability, backend execution or energy reduction.

## Mandatory rule for every remaining PR

Every PR from PR69 through PR85 must include all applicable test layers:

- unit tests,
- positive end-to-end integration tests,
- negative and boundary tests,
- a regression fixture for every corrected defect,
- sanitizer or race-detector execution for new native paths,
- security misuse tests for untrusted inputs and privileges,
- portability checks for platform-sensitive work,
- performance or energy evidence for hot paths,
- roadmap, feature-status and coverage-matrix updates.

No mandatory production test may be replaced by an unconditional skip.

## Detailed implementation strategy

| Planned PR | Status | Implementation scope | Mandatory tests and exit evidence |
| --- | --- | --- | --- |
| PR68 - Production test strategy, coverage audit and per-PR test contract | MERGED | Add the versioned test strategy, 27-area coverage matrix, PR template, CI guard and corrected roadmap count. | Validate matrix schema, unique IDs, status counts, PR69-PR85 mapping, CI integration and unsupported-claim boundaries. |
| PR69 - Module, import and package syntax with AST scaffold | PLANNED | Add versioned module, import, alias and package identity grammar; AST nodes; source provenance; stable diagnostics. Resolver behavior remains out of scope. | Positive grammar fixtures; malformed path, duplicate module, invalid alias and misplaced import negatives; source ranges; parser robustness and sanitizer tests. Exit: parser and AST represent module boundaries without claiming multi-file execution. |
| PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | PLANNED | Add canonical resolution, hermetic roots, package manifests, deterministic lockfiles, graph ordering, cycle detection, visibility and multi-file LLVM/native codegen. | Resolver unit tests; multi-file compile, run and native fixtures; cycles, missing imports, ambiguity, duplicate package and path-escape negatives; lockfile determinism; large graph stress. Exit: clean-checkout deterministic multi-file applications. |
| PR71 - Cross-mode semantic correctness and differential execution suite | PLANNED | Build a table-driven positive semantic matrix and normalized comparison across interpreter, bitcode and native binaries. | Cover all core types, expressions, calls, returns, branches, loops and arrays; compare output and exit status across modes; reject undefined behavior before lowering; establish code-coverage baseline. Exit: equivalent observable behavior across execution modes. |
| PR72 - Continuous fuzzing, full sanitizer and concurrency race hardening | PLANNED | Add scanner, parser, semantic and lowering fuzz targets; persisted corpora; full ASan, LSan and UBSan corpus; TSan runtime stress. | Pull-request fuzz smoke; scheduled extended fuzzing; all conformance, semantic, codegen, runtime and evidence fixtures under sanitizers; prolonged concurrent reset, registration, inference and snapshot stress. Exit: zero known sanitizer, race, hang or reproducible fuzz failures. |
| PR73 - Cross-platform toolchain matrix, CTest parity and reproducible builds | PLANNED | Qualify GCC and Clang, Linux x86-64 and arm64, macOS, Windows, supported LLVM versions, CTest parity, deterministic generation and install lifecycle. | Platform conformance and installed-consumer matrix; independent clean-build checksum comparison; normalized archive metadata; shared/static ABI consumers; generated Bison/Flex consistency. Exit: declared platforms produce equivalent consumable artifacts. |
| PR74 - Signed release and protected publication workflow | PLANNED | Add protected release environments, tag/version checks, OIDC signing, checksums, SBOM, provenance and attestations. | Reject unsigned, mismatched or mutated artifacts; signature and checksum verification; release dry-run; rollback and immutable manifest tests. Exit: verifiable releases cannot use an unprotected publication path. |
| PR75 - External vulnerability, SAST, dependency and license policy gate | PLANNED | Add C++ SAST, dependency and image CVE scanning, license policy, pinned actions and expiring vulnerability exceptions. | Detect intentionally vulnerable dependencies, secrets and prohibited licenses; reconcile SBOM packages; reject expired exceptions. Exit: unresolved high or critical findings block merge and release. |
| PR76 - Container and Kubernetes production hardening | PLANNED | Build minimal multi-arch non-root images with read-only filesystem, dropped capabilities, probes, resource policy, network policy and secure observability defaults. | Image build and CVE scan; non-root/read-only enforcement; ephemeral cluster deployment; health, metrics, shutdown and restart tests; malformed request and denied-network negatives. Exit: no privileged runtime requirement. |
| PR77 - Formatter and linter baseline | PLANNED | Add deterministic formatting, semantic-preserving lint rules, machine-readable diagnostics and strict repository enforcement. | Formatter idempotence; format-parse round trip for the full corpus; comment and string preservation; lint positive, negative and safe-fix fixtures; malformed input bounds. Exit: formatting never changes meaning. |
| PR78 - Syntax highlighting and LSP implementation | PLANNED | Add editor grammar and compiler-backed incremental diagnostics, hover, completion, definition and module navigation with bounded cancellation-aware requests. | Golden semantic-token and protocol JSON; partially typed documents; cancellation and stale-version tests; import navigation; large-document latency budget. Exit: tooling does not implement a divergent parser. |
| PR79 - Production backend and hardware qualification matrix | PLANNED | Declare production-supported backends and qualify real ONNX Runtime, TensorRT, OpenVINO, LibTorch and llama.cpp execution on declared CPU, GPU and available NPU hardware. | Real model execution with numerical tolerances; hardware selection; incompatible format, unavailable SDK, device loss and OOM negatives; no-output-copy and no-false-success checks. Skips remain unresolved evidence. Exit: every marketed backend has current live success evidence. |
| PR80 - Complete C3-ECO language blocks | PLANNED | Complete certification-oriented language constructs, AST, semantic validation, stable diagnostics and evidence declarations without granting certification. | Grammar and AST matrix; valid level-oriented examples; missing functional unit, invalid boundary, unsupported claim and inconsistent unit negatives; source-range and sanitizer tests. Exit: complete auditable measurement intent is expressible. |
| PR81 - Measured scoring, reports and eco-regression | PLANNED | Implement measured energy, carbon and cost calculations with factor provenance, uncertainty, data quality, baselines and deterministic reports. | Equation unit tests and independent calculations; golden reports; threshold pass/fail; missing sensor, stale factor, invalid unit and low-quality data negatives; numerical precision. Exit: measured, estimated and declared values are distinguished and reproducible. |
| PR82 - Authority-ready C3-ECO auditor bundle | PLANNED | Add signed manifests, source/binary/model/measurement lineage, retention, redaction, auditor verification and tamper detection. | Complete bundle validation; modified file, missing lineage, invalid signature and schema mismatch negatives; deterministic checksums; redaction leakage; clean-room replay. Exit: independent verification without trusting the build workspace. |
| PR83 - Generated MLIR dialect build integration | PLANNED | Generate dialect, operations, types and verifiers through TableGen; add parser/printer, installation and downstream consumption. | MLIR lit and FileCheck tests; verifier positive/negative cases; parse-print-parse round trip; installed consumer; generated-file freshness. Exit: generated dialect is the authoritative IR interface. |
| PR84 - Semantic IR to MLIR lowering and production backend handoff | PLANNED | Lower AST and SemanticIR into ShortHand MLIR, canonicalize and verify, lower core operations to LLVM and hand AI/Green AI operations to runtime contracts. | Pass-level lit tests; invalid shape and operation verifiers; interpreter-versus-MLIR/native differential suite; bridge metadata and typed-buffer tests; optimization semantic preservation. Exit: verified MLIR production compilation preserves behavior. |
| PR85 - Measured energy, performance and production RC gate | PLANNED | Add equivalent ShortHand and Python AI benchmarks, latency, throughput, memory and energy measurement, hardware metadata, uncertainty and final blocker aggregation. | Calibrate harness; validate identical input, model and output; repeat statistically; apply noise-aware thresholds; reject unsupported sensors or insufficient samples; run the complete clean-checkout release matrix with zero mandatory skips. Exit: zero production blockers and evidence-backed energy claims only. |

## Recommended next PR

Next recommended PR after PR #68:

PR69 - Module, import and package syntax with AST scaffold.

Reason: grammar, parser robustness and test governance are now explicit. Enterprise-scale programs next require versioned multi-file syntax and source provenance before deterministic resolution and code generation in PR70.

## Production readiness exit criteria

The final release gate must pass language compatibility, semantic equivalence, fuzzing and sanitizers, live backends, hardware routing, ABI, concurrency, portability, reproducibility, signing, security, deployment, tooling, C3-ECO, MLIR, performance and measured energy checks with zero mandatory skips.

## Remaining PR count fields

remaining_planned_prs_total_from_pr51_original: 29
remaining_planned_prs_total_from_pr51_reaudited: 35
remaining_planned_prs_after_pr61: 18
remaining_planned_prs_after_pr62: 17
remaining_planned_prs_after_pr63: 16
remaining_planned_prs_after_pr64: 15
remaining_planned_prs_after_pr65: 14
remaining_planned_prs_after_pr66: 13
remaining_planned_prs_after_pr67_original_estimate: 12
remaining_planned_prs_after_pr67_reaudited: 18
remaining_planned_prs_after_pr68: 17

## Historical compatibility record

The following exact milestones and recommendations are retained so earlier PR guards remain stable:

Language objectives consolidation applied in PR #57

Backend failure-mode finalization applied in PR #58

Runtime ABI and API stability applied in PR #59

Runtime state and thread-safety applied in PR #60

Production build packaging applied in PR #61

Prometheus scrape endpoint host adapter applied in PR #62

Recommended path from PR #51 onward: 29 PRs total.

PR59 - Runtime ABI and API version stability gate | MERGED

PR60 - Runtime state isolation and thread-safety policy | MERGED

PR61 - Production build packaging for runtime and AI bridge | MERGED

PR62 - Prometheus scrape endpoint host adapter | MERGED

After PR #62 is merged, approximately 17 implementation PRs remain.

Next recommended PR after PR #62:

PR63 - OTLP exporter adapter.

After PR #63 is merged, approximately 16 implementation PRs remain.

Next recommended PR after PR #63:

PR64 - AST source ranges across parser nodes.

After PR #65 is merged, approximately 14 implementation PRs remain.

Next recommended PR after PR #65:

PR66 - Full grammar and conformance matrix beta-0.2.

After PR #66 is merged, approximately 13 implementation PRs remain.

Next recommended PR after PR #66:

PR67 - Parser robustness and negative corpus hardening.

After PR #67 is merged, approximately 12 implementation PRs remain.

Next recommended PR after PR #67:

PR68 - Module/import/package design and parser scaffold.

PR67 - Parser robustness and negative corpus hardening

PR79 - MLIR lowering passes and production RC gate

The historical PR67 recommendation is superseded by the 2026-08-06 test re-audit. The new PR68 is the production test strategy and module work moves to PR69.
