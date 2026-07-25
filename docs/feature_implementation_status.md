# Feature Implementation Status

This tracker maps the feature plans in `docs/` to the current implementation. It should be updated whenever a feature plan is added, completed, or intentionally deferred.

## Current maturity

Current status: controlled beta / pilot language foundation.

ShortHand is not yet a fully production-ready industrial language for arbitrary enterprise AI applications. It has a working compiler foundation, AI/GreenAI syntax, semantic validation, fallback-aware runtime behavior, ONNX Runtime CPU execution when the SDK is configured, evidence reporting, CI validation, source-aware semantic diagnostics, runtime hook registries, runtime infer observability counters/JSON, semantic IR scaffolding, a beta EBNF grammar draft, conformance validation, source-level external-runtime lowering in the Makefile/CMake build path, a ShortHand MLIR dialect scaffold, C3-ECO candidate schema/claim-safety gates, candidate release SBOM/provenance generation, baseline release supply-chain scans, typed compiled-infer buffer bridge scaffolding, backend compatibility matrix gate, AI runtime bridge linkage guardrail, AI runtime execution adapter mapping, runtime AI bridge link-build validation, bridge-enabled compiled-hook routing into `AIRuntime::infer`, and release-readiness infrastructure. The remaining blockers are listed below and must be completed before any full enterprise production claim.

## Language feature plan status

Source plan: `docs/language_feature_implementation_plan.md`

| ID | Requirement | Implementation status | Evidence in repo | Production impact |
| --- | --- | --- | --- | --- |
| L1 | Keep model, tensor, contract, measurement, and infer metadata available to visitors | Implemented | `Compiler_new_ws/Short_Hand/src/ast/AST.h` | Enables compiler visitors to reason over AI/GreenAI constructs |
| L2 | Keep beta syntax stable for tensor/model/contract/measurement/infer | Implemented for current beta syntax | `docs/language_spec.md`, `docs/language_grammar_ebnf.md`, `scanner_parser/parser.yy`, `scanner_parser/scanner.ll`, `tests/conformance/manifest.txt` | Provides current beta language surface and a conformance contract for future changes |
| L3 | Reject infer when input tensor shape is incompatible with model input shape | Implemented | `Compiler_new_ws/Short_Hand/src/visitors/SemanticAnalyzer.cpp` | Prevents invalid AI programs from reaching runtime |
| L4 | Add negative tests for invalid AI programs | Implemented | `tests/semantic/invalid/ai_shape_mismatch.short`, `tests/semantic/invalid/ai_backend_mismatch.short`, `tests/semantic/invalid/ai_output_shape_mismatch.short`, `tests/conformance/manifest.txt` | Ensures semantic rejection is covered in CI and conformance validation |
| L5 | Emit runtime metadata for AI declarations and infer in compiled code instead of no-op lowering | Partial | `IR_Generator.cpp` emits LLVM metadata globals and compiled runtime hook calls; `scripts/apply_external_runtime_to_ir_source.sh` applies guarded source-level external hook lowering; `runtime/ShorthandRuntime.cpp` provides model/tensor/contract/measurement registries, infer status counters, bridge request JSON, typed float32 buffer bridge request handling, last-infer telemetry JSON, and observability JSON; `AI_Runtime.cpp` no longer exports duplicate compiled-hook C symbols; `runtime/AIRuntimeBridgeAdapter.*` maps runtime-hook records into `AI_Runtime` model/tensor/buffer types; `tests/codegen/test_runtime_ai_bridge_link_build.sh` validates that the runtime hook layer, adapter, and AI runtime core link together; `tests/codegen/test_runtime_ai_bridge_execution_path.sh` validates bridge-enabled routing into `AIRuntime::infer` with safe no-SDK fallback | Default builds compile `visitors/IR_Generator.cpp` directly after source-level runtime lowering and link native binaries through `libshorthand_runtime.a`; runtime hooks now expose bridge-ready observability, a typed buffer ABI, an AI_Runtime adapter mapping, an isolated AI runtime link-build gate, and a bridge-enabled execution route. The remaining work is proving SDK-backed success from the compiled-hook path with a real backend fixture |
| L6 | Source-aware diagnostics for semantic errors | Partial | `Diagnostics.cpp`, `SemanticAnalyzer.cpp`, `tests/diagnostics/test_source_diagnostics.sh`, `Compiler_new_ws/Short_Hand/src/semantic_ir/SemanticIR.h`, `docs/semantic_ir_and_diagnostics_plan.md` | Semantic errors now include source file, line, column, source line and caret for key AI/GreenAI anchors; semantic IR source range types exist; full AST span/range tracking remains open |
| L7 | Internal semantic IR for AI/GreenAI operations before LLVM/MLIR lowering | Scaffolded | `Compiler_new_ws/Short_Hand/src/semantic_ir/SemanticIR.h`, `mlir/include/ShortHand/IR/ShortHandDialect.td`, `mlir/include/ShortHand/IR/ShortHandOps.td`, `mlir/examples/ai_greenai_pipeline.mlir`, `docs/mlir_lowering_plan.md`, `scripts/check_mlir_foundation.sh` | Provides typed `ModelOp`, `TensorOp`, `InferOp`, `GreenAIContractOp`, and `GreenAIMeasurementOp` plus a matching MLIR dialect scaffold; parser/analyzer to MLIR integration remains open |

## C3-ECO certification language plan status

Source plan: `docs/c3eco_certification_language_upgrade_plan.md`

| ID | Requirement | Implementation status | Notes |
| --- | --- | --- | --- |
| C3L-1 | Certification declaration block | Open | Needed for product/version/scope identity |
| C3L-2 | Structured functional unit and workload profile | Open | Needed for denominator, success condition, workload, sampling and anti-gaming controls |
| C3L-3 | Typed boundary declaration | Open | Needed for included/excluded layers, materiality and third-party AI boundaries |
| C3L-4 | Measurement plan block | Open | Needed for instruments, MQ/DQ, factors, sampling, uncertainty and retention |
| C3L-5 | Resource and telemetry capture primitives | Partial | ONNX Runtime path emits latency/input/output telemetry JSON; source-level external runtime native linking, registry-backed hook state, runtime infer counters, compiled-infer bridge request JSON, typed buffer bridge request JSON, AI runtime bridge linkage guardrail, AI runtime execution adapter mapping, runtime AI bridge link-build validation, bridge-enabled AI runtime execution-path validation, and observability JSON exist; direct energy measurement and OTLP SDK export remain open |
| C3L-6 | AI lifecycle declaration | Open | Needed to separate provider/deployer/integrator responsibilities and model-training scope |
| C3L-7 | RAG, token, cache and routing metrics | Open | Needed for GenAI evidence, routing, prompt classes and usage accounting |
| C3L-8 | Carbon and cost calculation built-ins | Partial | Candidate workbook CSV calculates declared compute kWh and CO2e from existing measurement declarations; full cost/savings formulas and factors remain open |
| C3L-9 | Certification scoring and level estimator | Open | Must remain candidate-only unless external certifier signs |
| C3L-10 | Claim-safe report generation | Partial | `c3eco-report`, `c3eco-check`, `c3eco-workbook`, `scripts/generate_certification_bundle.sh`, C3-ECO schema files, and `scripts/check_c3eco_claims_and_schema.sh` validate candidate-only report structure and block unsupported public claims; Markdown report and full third-party JSON Schema validator remain open |
| C3L-11 | Quality/security/privacy/accessibility guardrails | Open | Needed to ensure efficiency is not achieved by weakening required quality or safety floors |
| C3L-12 | CI/CD and eco-regression gates | Partial | CI exercises candidate report/check/workbook paths, C3-ECO schema/claim-safety gate, enterprise hardening checks, source diagnostics, language conformance, runtime-library build checks, source-level external runtime native linking, registry-backed runtime hook checks, runtime infer observability checks, compiled-infer bridge checks, typed buffer bridge checks, backend compatibility matrix checks, AI runtime bridge linkage checks, AI runtime execution adapter checks, runtime AI bridge link-build checks, runtime AI bridge execution-path checks, MLIR foundation checks, release supply-chain checks, and the optional ONNX SDK gate skip path; release-to-release eco-regression is still open |

## Enterprise beta requirements status

Source plan: `docs/beta_enterprise_requirements.md`

| ID | Requirement group | Implementation status | Notes |
| --- | --- | --- | --- |
| R1 | Language contract | Partial | Current beta syntax is documented with a draft EBNF, conformance manifest, semantic IR scaffold, and MLIR dialect scaffold; complete grammar versioning and compatibility policy are still missing |
| R2 | Compiler build and validation | Implemented for current maturity | CI runs setup, strict validation, smoke tests, feature/enterprise hardening checks, language conformance, Makefile tests, sanitizer, CMake, and CTest |
| R3 | AI runtime behavior | Partial | Fallback is honest; ONNX Runtime CPU execution exists for SDK-enabled builds with an optional model-fixture gate; backend compatibility matrix gate validates compatibility tiers and fallback boundaries; `libshorthand_runtime.a` exports hook symbols and maintains model/tensor/contract/measurement registries plus infer counters, last-infer status/reason/backend, compiled and typed buffer bridge request JSON, telemetry JSON, and observability JSON; `AI_Runtime.cpp` no longer exports legacy compiled-hook C symbols; `runtime/AIRuntimeBridgeAdapter.*` maps the typed bridge into AI runtime model/tensor/buffer types; the runtime hook layer, adapter, and AI runtime core link together in an isolated CI probe; bridge-enabled builds route typed compiled-hook requests into `AIRuntime::infer`; SDK-backed success from this public hook path remains open |
| R4 | GreenAI and C3-ECO-aligned evidence | Partial | Evidence/report modes, semantic IR measurement scaffolding, MLIR GreenAI ops scaffold, bundle generator, schema files, and claim-safety gate exist; full authority-ready auditor handoff remains open |
| R5 | Security and supply-chain baseline | Partial | Security policy, SBOM plan, candidate SPDX SBOM generation, release provenance schema/output, source hash manifest, baseline secret scan, and release supply-chain gate exist; signed artifacts and external vulnerability scanner integration remain open |
| R6 | Developer experience | Partial | Build docs/examples, source-aware semantic diagnostics, grammar draft, conformance validation, and MLIR lowering plan exist; formatter, linter, editor tooling, and LSP remain open |
| R7 | Deployment and operations | Partial | Docker/Kubernetes scaffolds, observability plan, runtime hook library, runtime telemetry JSON fragments, and runtime observability JSON exist; production OTLP/Prometheus export remains open |
| R8 | Governance and release control | Partial | Release plans/status docs and candidate provenance output exist; full RFC workflow, protected release workflow, and signed release provenance are open |

## Enterprise release scorecard status

Source plan: `docs/enterprise_release_scorecard.md`

| Gate | Status | Notes |
| --- | --- | --- |
| G1-G6 build and test gates | Implemented for current CI | Current CI keeps setup, strict validation, smoke tests, Makefile tests, sanitizer, CMake, and CTest |
| G7 versioned language specification | Partial | Basic spec and EBNF grammar draft exist; formal versioning and compatibility policy are incomplete |
| G8 compatibility and deprecation policy | Partial | Compatibility notes exist; formal deprecation process incomplete |
| G9 conformance tests for all syntax | Partial | Conformance manifest and language correctness gate exist; not yet a full language matrix |
| G10 diagnostics with source locations | Partial | Semantic diagnostics now include file/line/column/source/caret for key AI/GreenAI anchors; semantic IR source range types exist; full AST range diagnostics remain open |
| G11 real AI backend execution | Partial | Real ONNX Runtime CPU execution is implemented behind `ONNXRUNTIME_ROOT`; optional SDK gate with small ONNX fixture exists and skips safely when SDK is absent; runtime hook observability, typed buffer bridge request scaffolding, backend matrix gate, linkage guardrails, AI runtime adapter mapping, bridge link-build validation, and bridge-enabled `AIRuntime::infer` routing exist, but SDK-backed compiled-hook success remains open |
| G12 fallback never claims executed inference | Implemented | Fallback, bridge-pending, and no-SDK bridge execution paths return `not_executed` and expose reason/status telemetry |
| G13 backend failure cases covered | Partial | Backend compatibility matrix, backend compatibility matrix gate, and negative tests exist; live execution matrix across all backends remains open |
| G14-G16 GreenAI measurement/evidence | Partial | Evidence modes, candidate bundle generator, semantic IR measurement op, runtime measurement registry, schema files, claim-safety gate, runtime observability JSON, and MLIR GreenAI op scaffold exist; full measurement plan and authority-ready bundle remain open |
| G17-G19 SBOM/signing/security scans | Partial | Dependency-free candidate SPDX SBOM generation, release provenance JSON, source hash manifest, and baseline secret/claim scan gate exist; signed artifacts and external dependency vulnerability scans remain open |
| G20 security disclosure | Implemented baseline | `SECURITY.md` exists |
| G21-G23 deployment and observability | Partial | Docker/Kubernetes scaffolds, observability plan, runtime hook library, runtime telemetry JSON, and runtime observability JSON exist; production OTLP SDK/exporter remains open |
| G24 governance/RFC | Partial | Release docs and candidate provenance exist; formal RFC workflow and protected release publication are open |
| G25 unsupported claims blocked | Partial | Candidate evidence outputs include disclaimers and the CI gate checks schema/claim safety; broader docs-wide claim scanning remains open |
| G26 MLIR dialect and lowering architecture | Scaffolded | `mlir/include/ShortHand/IR/ShortHandDialect.td`, `ShortHandOps.td`, example MLIR module, and `docs/mlir_lowering_plan.md` exist; real MLIR build integration and lowering passes remain open |

## Production blockers

These items must be completed before ShortHand can honestly be described as an industry-level production language for enterprise AI applications:

1. Real ONNX Runtime CPU backend execution with tests: SDK-backed execution is implemented and an optional SDK-enabled identity-model gate exists; default CI skips it when `ONNXRUNTIME_ROOT` is absent.
2. Compiled-code metadata/runtime lowering for `model`, `tensor`, `greenai_contract`, `greenai_measure`, and `infer`: metadata globals, runtime hook calls, runtime hook library, registry-backed hook state, source-level external runtime lowering, runtime infer observability, a default native-link test path, compiled bridge request JSON, typed buffer bridge request ABI, AI runtime linkage guardrail, AI runtime adapter mapping, isolated runtime/adapter/AI-core link-build validation, and bridge-enabled routing into `AIRuntime::infer` exist; remaining work is proving SDK-backed success from the compiled hook path.
3. Full backend compatibility and failure matrix: policy, semantic tests, and a backend compatibility matrix gate exist; live SDK execution across all marketed backends remains open.
4. Complete formal grammar and conformance test suite: EBNF draft, conformance manifest, and language correctness gate exist; full matrix coverage and versioning remain open.
5. Source-aware diagnostics with full AST file, line, column and range details across all parser and semantic errors: source-aware diagnostics and source range scaffolding exist; full AST parser-action integration remains open.
6. Automated SBOM generation and release signing: candidate SPDX SBOM, source hashes, provenance JSON, and baseline scan exist; signed release artifacts and protected release publication remain open.
7. Security/dependency scans in CI: baseline source secret scan exists; external dependency vulnerability scanning remains open.
8. Full authority-ready evidence bundle generator with auditor handoff. Candidate schema and claim-safety gates exist, but full independent validator/auditor workflow remains open.
9. Runtime observability implementation with real OTLP/Prometheus export. Runtime observability JSON exists; external exporter integration remains open.
10. Container/Kubernetes service hardening, health checks, and deployment validation.
11. Module/import/package model for enterprise-scale applications.
12. Developer tooling: formatter, linter, syntax highlighting, and LSP roadmap or implementation.
13. Full C3-ECO certification-aware language blocks, scoring, report generation and eco-regression gates.
14. MLIR dialect implementation for production-grade AI/tensor/GreenAI lowering: MLIR dialect scaffold and validation exist; generated C++ dialect, parser/printer tests, and lowering passes remain open.

## Review rule

Any PR that claims to make ShortHand production-ready must update this file and must either mark every production blocker as implemented with evidence or keep the claim conservative.
