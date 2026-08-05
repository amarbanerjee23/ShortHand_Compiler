# ShortHand production readiness PR plan

production_readiness_plan_version: 2026-08-02-pr67
PLAN_STATUS: active
BASELINE_AFTER_PR: 50
LAST_COMPLETED_PR: 67
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

## Purpose

This is the single planning source for moving ShortHand from controlled beta to enterprise production usage readiness. Every roadmap PR updates status, evidence, next work and remaining count. The mission remains defined by `shorthand.language.objectives.version: 2026-07-29-v1` with `production_claim: false`.

## Desired outcome definition

Enterprise production usage ready means versioned language behavior, reproducible build and release artifacts, honest AI execution evidence, hardware-aware routing, stable runtime contracts, multi-file programs, enterprise security and deployment, authority-ready C3-ECO evidence and integrated MLIR lowering.

Unsupported or unavailable paths must never report successful execution.

## Completed milestone record

### Audit correction applied in PR #51

PR51 added explicit ABI, runtime-state and parser-robustness milestones.

### Hardware routing expansion applied in PR #55

The roadmap contains 29 PRs from PR51 through PR79. Hardware detection never implies execution readiness.

### Language objectives consolidation applied in PR #57

The language mission and non-goals are versioned and guarded.

### Backend failure-mode finalization applied in PR #58

`shorthand.backend_failure_mode_matrix.v1` separates failure evidence from success evidence.

### Runtime ABI and API stability applied in PR #59

Runtime ABI `1.0.0` freezes exactly 25 external `short_*` symbols.

### Runtime state and thread-safety applied in PR #60

The public ABI is serialized and string snapshots are thread-local. Thread-safe does not mean multi-tenant isolated.

### Production build packaging applied in PR #61

runtime_packaging_status: installable_static_shared_and_consumer_checked

Static and shared runtime and AI bridge artifacts, CMake/pkg-config metadata and downstream consumers are guarded. Installable artifacts and successful consumer linking do not imply deployment readiness.

### Prometheus scrape endpoint host adapter applied in PR #62

A bounded loopback-default metrics host is installed and socket-tested. A loopback-default metrics endpoint does not imply authenticated, TLS-enabled or public-ingress readiness.

### OTLP exporter adapter applied in PR #63

Bounded OTLP/HTTP delivery, retries and rejection behavior are collector-tested. An OTLP endpoint returning HTTP 2xx proves transport acceptance only.

### AST source ranges applied in PR #64

Parser-produced AST nodes carry one-based inclusive ranges. Evidence includes `docs/ast_source_ranges.md`, `SourceRange.h`, `SourceRange.cpp`, parser/scanner location propagation, `tests/diagnostics/test_source_diagnostics.sh` and `scripts/check_ast_source_ranges.sh`.

Source ranges do not complete Unicode columns or imported-module provenance.

### Diagnostics coverage matrix applied in PR #65

Stable `SHDxxxx` codes cover parser, semantic, AI, Green AI and lowering-preflight stages. Warnings are printed without failing valid compilation. Errors remain non-zero. Undefined calls are rejected before LLVM IR generation.

Evidence:

- `docs/diagnostics_coverage_matrix.md`
- `Compiler_new_ws/Short_Hand/src/visitors/DiagnosticCodes.h`
- `tests/diagnostics/diagnostics_coverage_matrix.tsv`
- `scripts/check_diagnostics_coverage_matrix.sh`

### Full grammar and conformance matrix beta-0.2 applied in PR #66

Beta-0.2 replaced the draft grammar with a parser-accurate executable contract. It added parser-only validation, implementation-linked coverage, positive and negative fixtures, stable grammar rejection codes and Bison/Flex conflict checks while preserving the runtime ABI.

Evidence:

- `docs/language_grammar_ebnf.md`
- `docs/language_spec.md`
- `docs/language_versioning_and_conformance.md`
- `tests/conformance/grammar_matrix_beta_0_2.tsv`
- `tests/conformance/beta_0_2/`
- `scripts/check_grammar_conformance_matrix.sh`
- `Compiler_new_ws/Short_Hand/src/main.cpp`

### Parser robustness and negative corpus hardening applied in PR #67

The beta-0.2 parser is now guarded by bounded fail-fast handling without changing accepted syntax.

The contract provides:

1. a 4 MiB regular-source ceiling,
2. a 250,000 scanner-match ceiling,
3. a 16 KiB identifier, number and string-token ceiling,
4. a delimiter-nesting ceiling of 256,
5. lower-only environment overrides for constrained testing,
6. stable `SHD2004` through `SHD2010` resource and lexical codes,
7. explicit unterminated comment, unterminated string and unexpected-byte diagnostics,
8. a versioned malformed-input corpus,
9. repeated-output determinism checks,
10. wall-clock and virtual-memory bounds,
11. signal and sanitizer-marker rejection,
12. generated resource attacks and deterministic mutation smoke tests,
13. execution in normal negative tests and ASan/UBSan tests,
14. an unchanged beta-0.2 accepted grammar and runtime ABI.

Evidence:

- `docs/parser_robustness.md`
- `Compiler_new_ws/Short_Hand/src/parser/ParserLimits.h`
- `Compiler_new_ws/Short_Hand/src/scanner_parser/scanner.ll`
- `Compiler_new_ws/Short_Hand/src/main.cpp`
- `tests/parser/robustness/malformed_cases.tsv`
- `tests/parser/robustness/`
- `scripts/check_parser_robustness.sh`
- `tests/run_language_tests.sh`
- `.github/workflows/ci.yml`

The parser remains intentionally fail-fast. IDE-style multi-error recovery, Unicode display columns and module provenance remain outside this milestone.

## Current baseline after PR #67

- Language and conformance contracts remain `beta-0.2`.
- Backend availability, hardware routing and failure honesty are guarded.
- Runtime ABI remains 25 public symbols.
- Runtime state, packaging, Prometheus and OTLP evidence are guarded.
- AST source ownership is parser-propagated.
- Compiler diagnostics have stable codes, stages, severities and ranges.
- Every beta-0.2 grammar area has implementation-linked executable coverage.
- Malformed and adversarial parser inputs have bounded, deterministic failure behavior.
- Parser guardrails execute in normal and sanitizer-backed test paths.

ShortHand is still not production ready. Modules, release security, deployment, tooling, C3-ECO completion and MLIR integration remain open.

## Recommended remaining PR count

Recommended path from PR #51 onward: 29 PRs total.

After PR #67 is merged, approximately 12 implementation PRs remain.

## Next recommended PR

Next recommended PR after PR #67:

PR68 - Module/import/package design and parser scaffold.

Reason: the single-file beta grammar and parser are now versioned and bounded. Enterprise-scale applications next require an explicit module identity, import syntax, package boundary and source-provenance design before resolver and code-generation integration in PR69.

## PR roadmap table

| Planned PR | Status | Area |
| --- | --- | --- |
| PR51 - Production readiness plan and tracking contract | MERGED | Planning |
| PR52 - Backend live SDK matrix harness | MERGED | Backend coverage |
| PR53 - TensorRT optional live execution fixture | MERGED | Backend coverage |
| PR54 - OpenVINO optional live execution fixture | MERGED | Backend coverage |
| PR55 - LibTorch optional live execution fixture | MERGED | Backend coverage |
| PR56 - Hardware capability discovery and accelerator-aware routing | MERGED | Runtime hardware |
| PR57 - Llama.cpp optional live execution fixture | MERGED | Backend and objectives |
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
| PR68 - Module/import/package design and parser scaffold | PLANNED | Language scale |
| PR69 - Module resolver and codegen integration | PLANNED | Language scale |
| PR70 - Signed release and protected release workflow | PLANNED | Release |
| PR71 - External dependency vulnerability scan gate | PLANNED | Security |
| PR72 - Container and Kubernetes hardening | PLANNED | Deployment |
| PR73 - Formatter and linter baseline | PLANNED | Developer experience |
| PR74 - Syntax highlighting and LSP skeleton | PLANNED | Developer experience |
| PR75 - C3-ECO certification language blocks | PLANNED | C3-ECO language |
| PR76 - C3-ECO scoring, report generation, and eco-regression | PLANNED | C3-ECO evidence |
| PR77 - Authority-ready C3-ECO auditor bundle | PLANNED | C3-ECO evidence |
| PR78 - MLIR generated dialect build integration | PLANNED | MLIR |
| PR79 - MLIR lowering passes and production RC gate | PLANNED | MLIR and release |

## Production readiness exit criteria

Protected CI, language compatibility, backend honesty, hardware execution evidence, ABI and concurrency, packaging, observability, diagnostics, parser robustness, modules, security, deployment, tooling, C3-ECO and MLIR gates must all pass.

## Current remaining PR count field

remaining_planned_prs_total_from_pr51: 29
remaining_planned_prs_after_pr61: 18
remaining_planned_prs_after_pr62: 17
remaining_planned_prs_after_pr63: 16
remaining_planned_prs_after_pr64: 15
remaining_planned_prs_after_pr65: 14
remaining_planned_prs_after_pr66: 13
remaining_planned_prs_after_pr67: 12

## Historical PR62 recommendation markers

After PR #62 is merged, approximately 17 implementation PRs remain.

Next recommended PR after PR #62:

PR63 - OTLP exporter adapter.

Previous roadmap state: PR63 - OTLP exporter adapter | PLANNED

## Historical PR63 recommendation markers

After PR #63 is merged, approximately 16 implementation PRs remain.

Next recommended PR after PR #63:

PR64 - AST source ranges across parser nodes.

## Historical PR65 recommendation markers

After PR #65 is merged, approximately 14 implementation PRs remain.

Next recommended PR after PR #65:

PR66 - Full grammar and conformance matrix beta-0.2.

## Historical PR66 recommendation markers

After PR #66 is merged, approximately 13 implementation PRs remain.

Next recommended PR after PR #66:

PR67 - Parser robustness and negative corpus hardening.
