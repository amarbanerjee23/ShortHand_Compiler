# Feature Implementation Status

This tracker maps the feature plans in `docs/` to the current implementation. It should be updated whenever a feature plan is added, completed, or intentionally deferred.

## Current maturity

Current status: controlled beta / pilot language foundation.

ShortHand is not yet a fully production-ready industrial language for arbitrary enterprise AI applications. It has a working compiler foundation, AI/GreenAI syntax, semantic validation, fallback-aware runtime behavior, ONNX Runtime CPU execution when the SDK is configured, evidence reporting, CI validation, source-aware semantic diagnostics, runtime hook registries, and release-readiness infrastructure. The remaining blockers are listed below and must be completed before any full enterprise production claim.

## Language feature plan status

Source plan: `docs/language_feature_implementation_plan.md`

| ID | Requirement | Implementation status | Evidence in repo | Production impact |
| --- | --- | --- | --- | --- |
| L1 | Keep model, tensor, contract, measurement, and infer metadata available to visitors | Implemented | `Compiler_new_ws/Short_Hand/src/ast/AST.h` | Enables compiler visitors to reason over AI/GreenAI constructs |
| L2 | Keep beta syntax stable for tensor/model/contract/measurement/infer | Implemented for current beta syntax | `docs/language_spec.md`, `scanner_parser/parser.yy`, `scanner_parser/scanner.ll` | Provides current beta language surface |
| L3 | Reject infer when input tensor shape is incompatible with model input shape | Implemented | `Compiler_new_ws/Short_Hand/src/visitors/SemanticAnalyzer.cpp` | Prevents invalid AI programs from reaching runtime |
| L4 | Add negative tests for invalid AI programs | Implemented | `tests/semantic/invalid/ai_shape_mismatch.short`, `tests/semantic/invalid/ai_backend_mismatch.short`, `tests/semantic/invalid/ai_output_shape_mismatch.short` | Ensures semantic rejection is covered in CI |
| L5 | Emit runtime metadata for AI declarations and infer in compiled code instead of no-op lowering | Partial | `IR_Generator.cpp` emits LLVM metadata globals and compiled runtime hook calls; `runtime/ShorthandRuntime.cpp` now provides model/tensor/contract/measurement registries with status codes; `scripts/generate_external_runtime_ir_generator.sh` creates an external-runtime lowering variant; `tests/codegen/test_external_runtime_native.sh` verifies external declarations and native runtime-link execution | Demonstrates native AI/GreenAI hook execution through `libshorthand_runtime.a` and validates registry-backed hook behavior; remaining work is to make external-runtime lowering the default source implementation and route infer into `AI_Runtime` |
| L6 | Source-aware diagnostics for semantic errors | Partial | `Diagnostics.cpp`, `SemanticAnalyzer.cpp`, `tests/diagnostics/test_source_diagnostics.sh` | Semantic errors now include source file, line, column, source line and caret for key AI/GreenAI anchors; full AST span/range tracking remains open |

## C3-ECO certification language plan status

Source plan: `docs/c3eco_certification_language_upgrade_plan.md`

| ID | Requirement | Implementation status | Notes |
| --- | --- | --- | --- |
| C3L-1 | Certification declaration block | Open | Needed for product/version/scope identity |
| C3L-2 | Structured functional unit and workload profile | Open | Needed for denominator, success condition, workload, sampling and anti-gaming controls |
| C3L-3 | Typed boundary declaration | Open | Needed for included/excluded layers, materiality and third-party AI boundaries |
| C3L-4 | Measurement plan block | Open | Needed for instruments, MQ/DQ, factors, sampling, uncertainty and retention |
| C3L-5 | Resource and telemetry capture primitives | Partial | ONNX Runtime path now emits latency/input/output telemetry JSON; external runtime native linking and registry-backed hook state exist; direct energy measurement and OTLP SDK export remain open |
| C3L-6 | AI lifecycle declaration | Open | Needed to separate provider/deployer/integrator responsibilities and model-training scope |
| C3L-7 | RAG, token, cache and routing metrics | Open | Needed for GenAI evidence, routing, prompt classes and usage accounting |
| C3L-8 | Carbon and cost calculation built-ins | Partial | Candidate workbook CSV calculates declared compute kWh and CO2e from existing measurement declarations; full cost/savings formulas and factors remain open |
| C3L-9 | Certification scoring and level estimator | Open | Must remain candidate-only unless external certifier signs |
| C3L-10 | Claim-safe report generation | Partial | `c3eco-report`, `c3eco-check`, `c3eco-workbook`, and `scripts/generate_certification_bundle.sh` exist with candidate-only disclaimers; Markdown report and full schema remain open |
| C3L-11 | Quality/security/privacy/accessibility guardrails | Open | Needed to ensure efficiency is not achieved by weakening required quality or safety floors |
| C3L-12 | CI/CD and eco-regression gates | Partial | CI exercises candidate report/check/workbook paths, enterprise hardening checks, source diagnostics, runtime-library build checks, external runtime native linking, registry-backed runtime hook checks, and the optional ONNX SDK gate skip path; release-to-release eco-regression is still open |

## Enterprise beta requirements status

Source plan: `docs/beta_enterprise_requirements.md`

| ID | Requirement group | Implementation status | Notes |
| --- | --- | --- | --- |
| R1 | Language contract | Partial | Current beta syntax is documented, but complete grammar/versioned standard is still missing |
| R2 | Compiler build and validation | Implemented for current maturity | CI runs setup, strict validation, smoke tests, feature/enterprise hardening checks, Makefile tests, sanitizer, CMake, and CTest |
| R3 | AI runtime behavior | Partial | Fallback is honest; ONNX Runtime CPU execution exists for SDK-enabled builds with an optional model-fixture gate; `libshorthand_runtime.a` exports hook symbols and now maintains model/tensor/contract/measurement registries; generated external-runtime lowering is verified through a native-link test path |
| R4 | GreenAI and C3-ECO-aligned evidence | Partial | Evidence/report modes and bundle generator exist; full authority-ready evidence schema remains open |
| R5 | Security and supply-chain baseline | Partial | Security policy and SBOM plan exist; automated SBOM/signing are still open |
| R6 | Developer experience | Partial | Build docs/examples and source-aware semantic diagnostics exist; formatter, linter, editor tooling, and LSP remain open |
| R7 | Deployment and operations | Partial | Docker/Kubernetes scaffolds and observability plan exist; runtime telemetry JSON now exists, but production OTLP/Prometheus export remains open |
| R8 | Governance and release control | Partial | Release plans/status docs exist; full RFC workflow and automated release provenance are open |

## Enterprise release scorecard status

Source plan: `docs/enterprise_release_scorecard.md`

| Gate | Status | Notes |
| --- | --- | --- |
| G1-G6 build and test gates | Implemented for current CI | Current CI keeps setup, strict validation, smoke tests, Makefile tests, sanitizer, CMake, and CTest |
| G7 versioned language specification | Partial | Basic spec exists; full grammar/versioning incomplete |
| G8 compatibility and deprecation policy | Partial | Compatibility notes exist; formal deprecation process incomplete |
| G9 conformance tests for all syntax | Partial | Positive/negative tests exist; not full language matrix |
| G10 diagnostics with source locations | Partial | Semantic diagnostics now include file/line/column/source/caret for key AI/GreenAI anchors; full AST range diagnostics remain open |
| G11 real AI backend execution | Partial | Real ONNX Runtime CPU execution is implemented behind `ONNXRUNTIME_ROOT`; optional SDK gate with small ONNX fixture exists and skips safely when SDK is absent |
| G12 fallback never claims executed inference | Implemented | Fallback path is deterministic and reported honestly |
| G13 backend failure cases covered | Partial | Backend compatibility matrix and negative tests exist; live execution matrix across all backends remains open |
| G14-G16 GreenAI measurement/evidence | Partial | Evidence modes, candidate bundle generator, and runtime measurement registry exist; full measurement plan and authority-ready bundle remain open |
| G17-G19 SBOM/signing/security scans | Open | Plans exist but automation is not implemented |
| G20 security disclosure | Implemented baseline | `SECURITY.md` exists |
| G21-G23 deployment and observability | Partial | Docker/Kubernetes scaffolds, observability plan, runtime hook library, and runtime telemetry JSON exist; production OTLP SDK/exporter remains open |
| G24 governance/RFC | Partial | Release docs exist; formal RFC workflow is open |
| G25 unsupported claims blocked | Partial | Docs and reports avoid claims; automated claim gate is still open |

## Production blockers

These items must be completed before ShortHand can honestly be described as an industry-level production language for enterprise AI applications:

1. Real ONNX Runtime CPU backend execution with tests: SDK-backed execution is implemented and an optional SDK-enabled identity-model gate exists; default CI skips it when `ONNXRUNTIME_ROOT` is absent.
2. Compiled-code metadata/runtime lowering for `model`, `tensor`, `greenai_contract`, `greenai_measure`, and `infer`: metadata globals, runtime hook calls, runtime hook library, registry-backed hook state, and an external-runtime native-link test path exist; remaining work is to replace the transitional generated external-runtime lowering with direct source-level external hook declarations in `IR_Generator.cpp` and route compiled infer into `AI_Runtime`.
3. Full backend compatibility and failure matrix: policy and semantic tests exist; live SDK execution across all marketed backends remains open.
4. Complete formal grammar and conformance test suite.
5. Source-aware diagnostics with full AST file, line, column and range details across all parser and semantic errors.
6. Automated SBOM generation and release signing.
7. Security/dependency scans in CI.
8. Full authority-ready evidence bundle generator with schema validation and auditor handoff.
9. Runtime observability implementation with real OTLP/Prometheus export, not only JSON fragments.
10. Container/Kubernetes service hardening, health checks, and deployment validation.
11. Module/import/package model for enterprise-scale applications.
12. Developer tooling: formatter, linter, syntax highlighting, and LSP roadmap or implementation.
13. Full C3-ECO certification-aware language blocks, scoring, report generation and eco-regression gates.
14. MLIR dialect implementation for production-grade AI/tensor/GreenAI lowering.

## Review rule

Any PR that claims to make ShortHand production-ready must update this file and must either mark every production blocker as implemented with evidence or keep the claim conservative.
