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

ShortHand must become a production-grade compiled AI language that lets engineers build and deploy AI software without Python. It must provide predictable language semantics, honest hardware-aware execution, lower runtime overhead, measurable energy efficiency, portable release artifacts, enterprise security, and auditable Green AI evidence.

Unsupported or unavailable execution paths must never report success. A skipped dependency, absent accelerator, source-pattern check, or transport acceptance response is not production execution evidence.

## Purpose

This is the single planning source for moving ShortHand from controlled beta to enterprise production usage readiness. Every roadmap PR must update status, test evidence, next work and remaining count. The language mission remains versioned in `docs/language_objectives.md` with `production_claim: false` until the final release-candidate gate succeeds.

## Test audit correction applied in PR #68

The PR67 roadmap estimated 12 remaining PRs. A full compiler and CI audit found that this omitted six production-critical workstreams:

1. governed compiler test coverage and a mandatory per-PR test contract,
2. interpreter, LLVM and native differential semantic correctness,
3. continuous fuzzing, full sanitizer coverage and race detection,
4. cross-platform toolchain qualification and reproducible builds,
5. real live-backend and device qualification,
6. measured ShortHand-versus-Python performance and energy evidence.

After PR #67, 18 production-readiness PRs are required.

PR68 establishes the test strategy and coverage matrix. After PR #68 is merged, 17 implementation PRs remain.

The revised path contains 35 PRs from PR51 through PR85. The extra work is necessary because green Linux CI alone cannot prove enterprise language correctness, portability, backend execution or energy reduction.

## Current baseline after PR #68

Implemented or strongly guarded:

- beta-0.2 parser-accurate grammar and conformance matrix,
- bounded malformed-input behavior and parser resource ceilings,
- stable coded diagnostics and source ranges,
- selected AI semantic rejection fixtures,
- LLVM metadata and external runtime-linking paths,
- honest fallback and unavailable-backend behavior,
- hardware discovery and execution-ready routing policy,
- runtime ABI 1.0.0 with 25 public symbols,
- serialized public ABI and thread-local snapshots,
- Linux runtime packaging consumers,
- Prometheus and OTLP host adapters,
- candidate C3-ECO schema and claim-safety evidence,
- ASan, LSan and UBSan parser smoke coverage,
- compiler test strategy, coverage matrix and PR test template.

Still open or partial:

- multi-file modules, packages and deterministic dependency resolution,
- language-wide semantic equivalence across interpreter and compiled outputs,
- full-corpus sanitizer, fuzzing and ThreadSanitizer coverage,
- multi-platform and reproducible release builds,
- signed releases and external vulnerability enforcement,
- hardened deployment,
- formatter, linter and LSP support,
- live execution qualification for each production-supported backend,
- complete C3-ECO measurement and authority evidence,
- generated MLIR dialect and production lowering,
- measured energy reduction against equivalent Python workloads,
- final zero-skip release-candidate gate.

ShortHand remains `controlled_beta` and `production_claim: false`.

## Mandatory rule for every remaining PR

Every PR from PR69 through PR85 must include applicable unit, positive integration, negative boundary, regression, sanitizer, security, portability and performance tests. The PR must update:

- `docs/feature_implementation_status.md`,
- this roadmap,
- `tests/coverage/compiler_test_coverage_matrix.tsv`,
- conformance or compatibility contracts affected by the change.

No mandatory test may be replaced by an unconditional skip.

## Detailed implementation strategy

### PR68 - Production test strategy, coverage audit and per-PR test contract

Status: MERGED in the planned post-merge state.

Implementation:

- versioned compiler test strategy,
- 27-area coverage matrix,
- PR template requiring explicit test evidence,
- CI and enterprise guard for matrix integrity,
- corrected remaining-PR count.

Tests:

- matrix schema and unique-ID validation,
- exact implemented, partial and open counts,
- roadmap PR69 through PR85 anchors,
- CI integration and unsupported-claim checks.

Exit evidence:

- `docs/compiler_test_strategy.md`,
- `tests/coverage/compiler_test_coverage_matrix.tsv`,
- `scripts/check_compiler_test_strategy.sh`.

### PR69 - Module, import and package syntax with AST scaffold

Implementation:

- versioned `module`, `import` and package identity grammar,
- AST nodes carrying source provenance,
- stable diagnostics for malformed and duplicate declarations,
- beta language-version update only if syntax compatibility policy requires it.

Required tests:

- grammar matrix rows for every syntax form,
- positive single-module and import fixtures,
- malformed path, duplicate module, invalid alias and misplaced import negatives,
- source-range and diagnostic-code checks,
- parser resource and sanitizer execution for import-heavy files.

Exit condition: parser and AST represent module boundaries without resolving files or claiming multi-file execution.

### PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen

Implementation:

- canonical module resolution,
- package manifest and deterministic lockfile,
- cycle and ambiguity detection,
- hermetic search roots and path traversal prevention,
- symbol visibility and multi-file LLVM/native integration.

Required tests:

- resolver unit tests for canonicalization and graph order,
- multi-file parse, compile, run and native fixtures,
- cycle, missing import, ambiguous symbol, duplicate package and path-escape negatives,
- lockfile determinism and cache invalidation tests,
- large module graph stress and sanitizer coverage.

Exit condition: a clean checkout builds deterministic multi-file applications without undeclared host-path dependencies.

### PR71 - Cross-mode semantic correctness and differential execution suite

Implementation:

- table-driven semantic obligations for core types, expressions, functions and control flow,
- normalized output comparison across interpreter, LLVM bitcode and native binaries,
- positive semantic matrix complementing existing negative cases,
- golden diagnostic and exit-status contract.

Required tests:

- every beta language construct executes in at least one positive fixture,
- interpreter, bitcode and native outputs match,
- arithmetic, comparison, branching, loops, arrays, calls and returns cover boundary values,
- undefined behavior is rejected before lowering,
- code coverage baseline for parser, semantic analyzer and codegen.

Exit condition: accepted source has equivalent observable behavior in every supported execution mode.

### PR72 - Continuous fuzzing, full sanitizer and concurrency race hardening

Implementation:

- coverage-guided scanner, parser, semantic and IR-lowering fuzz targets,
- persisted seed corpus from conformance and bug fixtures,
- full-corpus ASan, LSan and UBSan execution,
- ThreadSanitizer runtime tests,
- bounded OOM, deep graph and long-token stress.

Required tests:

- fuzz target smoke in pull-request CI,
- scheduled extended fuzz jobs with artifacted corpora,
- all conformance, semantic, codegen, runtime and evidence fixtures under sanitizers,
- prolonged concurrent reset, registration, inference and snapshot stress under TSan,
- regression fixture for every discovered crash or leak.

Exit condition: zero known sanitizer findings, races, hangs or reproducible fuzz crashes.

### PR73 - Cross-platform toolchain matrix, CTest parity and reproducible builds

Implementation:

- GCC and Clang matrix,
- Linux x86-64 and arm64, macOS arm64 and x86-64 where supported, Windows x86-64,
- LLVM version support policy,
- CTest parity with mandatory Makefile tests,
- deterministic generated parser and reproducible artifact controls,
- install, upgrade, uninstall and downstream consumer workflows.

Required tests:

- platform build, conformance and installed-consumer matrix,
- independent clean-build checksum comparison,
- `SOURCE_DATE_EPOCH` and normalized archive metadata checks,
- static and shared library ABI consumer tests,
- generated Bison/Flex output consistency.

Exit condition: declared release platforms build and consume equivalent artifacts from clean environments.

### PR74 - Signed release and protected publication workflow

Implementation:

- protected release environment,
- tag and version consistency checks,
- OIDC-based artifact signing,
- checksums, SBOM, provenance and SLSA-style attestations,
- release dry-run and rollback procedure.

Required tests:

- unsigned or mismatched artifacts are rejected,
- signature and checksum verification tests,
- tag/version mismatch negative fixtures,
- release dry-run without publication,
- immutable artifact manifest comparison.

Exit condition: release artifacts are verifiable, traceable and cannot be published through an unprotected path.

### PR75 - External vulnerability, SAST, dependency and license policy gate

Implementation:

- CodeQL or equivalent C++ SAST,
- OSV/Trivy or equivalent dependency and artifact scanning,
- license allowlist and prohibited-license policy,
- pinned action and dependency versions,
- vulnerability exception format with expiry and ownership.

Required tests:

- intentionally vulnerable dependency fixture is detected,
- secret and private-key negative fixtures,
- prohibited license fixture,
- SBOM-to-scanner package reconciliation,
- expired exception rejection.

Exit condition: high and critical unresolved findings block merge and release.

### PR76 - Container and Kubernetes production hardening

Implementation:

- minimal multi-architecture non-root image,
- read-only filesystem and dropped capabilities,
- resource requests, limits, probes and graceful shutdown,
- network policies and secure observability defaults,
- Helm or Kustomize deployment contract.

Required tests:

- image build and vulnerability scan,
- non-root and read-only enforcement,
- kind or equivalent ephemeral-cluster deployment,
- health, metrics, shutdown and restart tests,
- oversized request, malformed telemetry and denied-network negatives.

Exit condition: hardened deployment passes isolated cluster validation without privileged execution.

### PR77 - Formatter and linter baseline

Implementation:

- deterministic formatter for accepted syntax,
- semantic-preserving lint rules,
- machine-readable diagnostics and safe-fix boundaries,
- strict repository formatting enforcement.

Required tests:

- formatter idempotence,
- format then parse round trip for the full conformance corpus,
- comments and string literal preservation,
- lint positive, negative and autofix fixtures,
- malformed input bounded failure.

Exit condition: formatting never changes program meaning and lint output is stable.

### PR78 - Syntax highlighting and LSP implementation

Implementation:

- editor grammar,
- incremental document parsing,
- coded diagnostics, hover, completion, definition and module navigation,
- cancellation, bounded requests and versioned protocol behavior.

Required tests:

- golden semantic-token and protocol JSON,
- malformed and partially typed documents,
- cancellation and stale-document version tests,
- module import navigation and source provenance,
- large-document latency budget.

Exit condition: editor tooling consumes the compiler contracts without implementing a divergent language parser.

### PR79 - Production backend and hardware qualification matrix

Implementation:

- explicit list of production-supported backends,
- real ONNX Runtime, TensorRT, OpenVINO, LibTorch and llama.cpp execution where declared,
- CPU, GPU and available NPU qualification,
- model fixture hashes, numerical tolerances and telemetry evidence,
- self-hosted or controlled hardware-runner policy.

Required tests:

- real model execution and numerical-output validation per supported backend,
- hardware detection-to-selection verification,
- incompatible format, unavailable SDK, device loss and OOM negatives,
- no-output-copy and no-false-success assertions,
- skips reported as unresolved evidence, never success.

Exit condition: every backend marketed as supported has current live success evidence on declared hardware.

### PR80 - Complete C3-ECO language blocks

Implementation:

- complete certification-oriented language constructs,
- AST, semantic validation and stable diagnostics,
- versioned boundary, functional unit, measurement quality, data quality and evidence declarations.

Required tests:

- grammar and AST matrix,
- valid Bronze through Diamond evidence examples without granting certification,
- missing functional unit, invalid boundary, unsupported claim and inconsistent unit negatives,
- source ranges and sanitizer coverage.

Exit condition: the language can express complete auditable measurement intent without overclaiming certification.

### PR81 - Measured scoring, reports and eco-regression

Implementation:

- measured energy, carbon and cost pipeline,
- provenance for factors and units,
- uncertainty and data-quality handling,
- baseline comparison and regression budgets,
- deterministic report and workbook generation.

Required tests:

- equation unit tests and independent reference calculations,
- deterministic golden reports,
- threshold pass and fail fixtures,
- missing sensor, stale factor, invalid unit and low-quality data negatives,
- schema compatibility and numerical precision tests.

Exit condition: reports distinguish measured, estimated and declared values and reproduce their calculations.

### PR82 - Authority-ready C3-ECO auditor bundle

Implementation:

- signed evidence manifest,
- source, binary, model and measurement lineage,
- redaction and retention policy,
- auditor verification command and tamper detection.

Required tests:

- complete bundle validation,
- modified file, missing lineage, invalid signature and schema mismatch negatives,
- deterministic manifest and checksum tests,
- redaction leakage tests,
- clean-room auditor replay.

Exit condition: an independent auditor can verify evidence without trusting the build workspace.

### PR83 - Generated MLIR dialect build integration

Implementation:

- TableGen-generated dialect, operations, types and verifiers,
- CMake installation and downstream consumption,
- parser and printer support.

Required tests:

- MLIR lit and FileCheck tests,
- operation verifier positive and negative cases,
- parse-print-parse round trip,
- installed dialect consumer build,
- generated-file freshness guard.

Exit condition: the generated dialect is the authoritative intermediate representation interface.

### PR84 - Semantic IR to MLIR lowering and production backend handoff

Implementation:

- AST and SemanticIR lowering into ShortHand MLIR,
- canonicalization and verification passes,
- LLVM lowering for core language operations,
- AI runtime and Green AI operation handoff,
- stable lowering diagnostics.

Required tests:

- pass-level lit tests,
- invalid operation and shape verifier tests,
- interpreter versus MLIR/native differential suite,
- AI runtime metadata and typed-buffer bridge tests,
- optimization semantic-preservation tests.

Exit condition: production compilation uses verified MLIR lowering without changing observable behavior.

### PR85 - Measured energy, performance and production RC gate

Implementation:

- representative equivalent ShortHand and Python AI workloads,
- compiler time, runtime latency, throughput, memory and energy measurement,
- RAPL, NVML, platform sensor or laboratory adapter policy,
- statistical repetition, hardware metadata and uncertainty,
- release scorecard aggregation and zero-skip blocker gate.

Required tests:

- benchmark harness correctness and calibration fixtures,
- equivalent input, model and output validation across ShortHand and Python,
- regression thresholds with noise handling,
- unsupported-sensor and insufficient-sample negatives,
- complete clean-checkout build, install, conformance, backend, security, deployment, tooling, C3-ECO and MLIR matrix.

Exit condition: all mandatory tests pass with zero unresolved production blockers, and any published lower-energy claim is supported by reproducible evidence rather than assumption.

## PR roadmap table

| Planned PR | Status | Area |
| --- | --- | --- |
| PR51 - Production readiness plan and tracking contract | MERGED | Planning |
| PR52 - Backend live SDK matrix harness | MERGED | Backend coverage |
| PR53 - TensorRT optional live execution fixture | MERGED | Backend coverage |
| PR54 - OpenVINO optional live execution fixture | MERGED | Backend coverage |
| PR55 - LibTorch optional live execution fixture | MERGED | Backend coverage |
| PR56 - Hardware capability discovery and accelerator-aware routing | MERGED | Runtime hardware |
| PR57 - Llama.cpp optional live execution fixture and objectives | MERGED | Backend and objectives |
| PR58 - Backend failure-mode matrix finalization | MERGED | Runtime reliability |
| PR59 - Runtime ABI and API version stability gate | MERGED | Runtime contract |
| PR60 - Runtime state isolation and thread-safety policy | MERGED | Runtime reliability |
| PR61 - Production build packaging for runtime and AI bridge | MERGED | Build |
| PR62 - Prometheus scrape endpoint host adapter | MERGED | Operations |
| PR63 - OTLP exporter adapter | MERGED | Operations |
| PR64 - AST source ranges across parser nodes | MERGED | Diagnostics |
| PR65 - Diagnostics coverage matrix | MERGED | Diagnostics |
| PR66 - Full grammar and conformance matrix beta-0.2 | MERGED | Language contract |
| PR67 - Parser robustness and negative corpus hardening | MERGED | Language robustness |
| PR68 - Production test strategy, coverage audit and PR test contract | MERGED | Quality governance |
| PR69 - Module, import and package syntax with AST scaffold | PLANNED | Language scale |
| PR70 - Deterministic module resolver, package manifest, lockfile and multi-file codegen | PLANNED | Language scale |
| PR71 - Cross-mode semantic correctness and differential execution suite | PLANNED | Compiler correctness |
| PR72 - Continuous fuzzing, full sanitizer and concurrency race hardening | PLANNED | Compiler robustness |
| PR73 - Cross-platform toolchain matrix, CTest parity and reproducible builds | PLANNED | Portability |
| PR74 - Signed release and protected publication workflow | PLANNED | Release |
| PR75 - External vulnerability, SAST, dependency and license policy gate | PLANNED | Security |
| PR76 - Container and Kubernetes production hardening | PLANNED | Deployment |
| PR77 - Formatter and linter baseline | PLANNED | Developer experience |
| PR78 - Syntax highlighting and LSP implementation | PLANNED | Developer experience |
| PR79 - Production backend and hardware qualification matrix | PLANNED | AI execution |
| PR80 - Complete C3-ECO language blocks | PLANNED | C3-ECO language |
| PR81 - Measured scoring, reports and eco-regression | PLANNED | C3-ECO evidence |
| PR82 - Authority-ready C3-ECO auditor bundle | PLANNED | C3-ECO evidence |
| PR83 - Generated MLIR dialect build integration | PLANNED | MLIR |
| PR84 - Semantic IR to MLIR lowering and production backend handoff | PLANNED | MLIR and codegen |
| PR85 - Measured energy, performance and production RC gate | PLANNED | Evidence and release |

## Recommended next PR

Next recommended PR after PR #68:

PR69 - Module, import and package syntax with AST scaffold.

Reason: the parser, grammar, robustness and test-governance foundations are now explicit. Enterprise-scale applications next require versioned multi-file syntax and source provenance before deterministic resolution and code generation in PR70.

## Production readiness exit criteria

Protected CI, language compatibility, semantic equivalence, parser and compiler robustness, backend honesty, live hardware execution, ABI and concurrency, portability, reproducibility, signed releases, vulnerability policy, deployment, tooling, C3-ECO, MLIR, performance and measured energy gates must all pass.

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

## Historical recommendation markers

Recommended path from PR #51 onward: 29 PRs total.

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

The historical PR67 recommendation is superseded by the 2026-08-06 test re-audit. The new PR68 is the production test strategy and the module work moves to PR69.
