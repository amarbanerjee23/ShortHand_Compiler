# Enterprise Release Readiness Strategy

ShortHand is currently a controlled beta with `production_claim: false`. This document defines the strategy for moving it toward enterprise use for AI software development without making unsupported public-readiness, certification, inherent-greenness or absolute defect-freedom claims.

## Current status

Current maturity: controlled enterprise beta (ER3), not an enterprise release candidate.

The repository has useful foundations:

- C++17, Flex/Bison, Make/CMake, and LLVM-based implementation path.
- GreenAI evidence syntax and report generation.
- AI model, tensor, and inference syntax.
- Live ONNX Runtime CPU numerical qualification for the narrow `linux-x64-cpu-v1` support set, plus deterministic fail-closed fallback behavior.
- CI gates for setup, strict validation, smoke tests, Makefile tests, sanitizer checks, CMake build, and CTest.
- Explicit evidence-only policy and complete G1-G14/A-K/S9/S12 traceability for C3-ECO-aligned candidate reports.

It is not yet ready for general enterprise production use because composite execution lowering, public authenticated service ingress, full C3-ECO preparation, generated MLIR lowering, representative AI workload, measured performance/energy, final RC aggregate and protected publication exercise remain incomplete. The process-scoped `shorthand.serving.runtime.v1` contract is implemented without claiming a hardened public service.

## Enterprise target

The target is to make ShortHand a governed AI systems language with:

1. Simple language-level syntax for AI workloads.
2. Real native backend execution through supported C++ AI runtimes.
3. Strong compiler correctness and diagnostics.
4. Evidence-first GreenAI and C3-ECO-aligned telemetry.
5. Enterprise deployment paths for containers and Kubernetes/OpenShift.
6. Secure release engineering, SBOM, signed artifacts, and reproducible validation.
7. A stable language specification, compatibility policy, and long-term support model.

## Non-negotiable acceptance rule

A release must not be described as enterprise-ready until all mandatory gates in `docs/enterprise_release_scorecard.md` pass and the release has an auditable evidence bundle.

Allowed interim wording:

- internal engineering review
- research artifact
- pilot candidate
- enterprise-readiness roadmap
- evidence-only GreenAI reporting

Disallowed unsupported wording:

- complete enterprise rollout status
- official external certification status
- absolute defect-freedom
- executed AI inference when only fallback was used
- measured energy or carbon values when the tools were unavailable

## Workstream 1: Language specification and compatibility

Goal: freeze the user-facing language contract.

Required deliverables:

- Versioned language specification.
- Full grammar reference.
- Stable syntax for model, tensor, inference, GreenAI contract, GreenAI measurement, reports, and runtime configuration.
- Type system rules.
- Module/import policy.
- Error and diagnostic policy.
- Compatibility and deprecation policy.
- Conformance test suite for every grammar and semantic rule.

Exit criteria:

- Every accepted syntax form has a positive test.
- Every rejected syntax form has a negative test.
- Diagnostics include file, line, and clear remediation text.
- Breaking language changes require an RFC and migration note.

## Workstream 2: Compiler correctness and safety

Goal: make the compiler reliable enough for sustained internal pilots.

Required deliverables:

- Parser and scanner regression tests.
- Semantic analyzer test matrix.
- Interpreter versus compiled-output differential tests.
- LLVM IR golden-output tests.
- Fuzzing for parser and semantic validation.
- Sanitizer clean test runs.
- No-throw validation in code paths compiled with LLVM flags that may disable exceptions.
- Deterministic builds.
- Coverage reporting.

Exit criteria:

- CI passes on a clean Ubuntu runner.
- Sanitizer tests pass.
- No compiler crashes on invalid input corpus.
- All invalid programs fail with controlled diagnostics.

## Workstream 3: Real AI runtime execution

Goal: move beyond fallback-only execution.

Required deliverables:

- ONNX Runtime CPU backend as the first real backend.
- Backend availability detection.
- Model loading and shape validation.
- Tensor binding and output handling.
- Runtime error mapping to ShortHand diagnostics.
- Backend selection policy.
- Tests for successful inference and failure cases.
- Evidence fields that distinguish executed inference from fallback.

Follow-on backends:

- ONNX Runtime CUDA.
- TensorRT or ONNX Runtime TensorRT execution provider.
- OpenVINO.
- LibTorch.
- llama.cpp for local LLM-style inference experiments.

Exit criteria:

- At least one real backend executes a model in CI or in a documented optional integration workflow.
- Fallback is never reported as successful inference.
- Missing SDKs are disclosed as skipped optional checks.

## Workstream 4: GreenAI and C3-ECO-aligned evidence

Goal: make sustainability evidence traceable and auditable.

Required deliverables:

- Functional unit declaration.
- Boundary declaration.
- Measurement plan file.
- Raw telemetry references.
- Energy per functional unit.
- Carbon per functional unit.
- Measurement quality and data quality fields.
- Uncertainty reporting.
- Token, batch, cache, retrieval, and failed-run metrics for AI workloads.
- Third-party model/API boundary disclosure.
- Evidence retention policy.

Exit criteria:

- Reports never fabricate telemetry.
- Missing tools are reported as unavailable.
- Offsets and avoided impact are never subtracted from the base footprint.
- The report clearly states that it is evidence only unless reviewed by an external certification authority.

## Workstream 5: Security and supply chain

Goal: make the toolchain safe enough for enterprise pilot use.

Required deliverables:

- Security policy.
- Vulnerability disclosure process.
- SBOM for release artifacts.
- Dependency scanning.
- Secret scanning.
- Static analysis.
- Signed release artifacts.
- Release provenance.
- Threat model for compiler, runtime, model loading, and generated artifacts.
- Sandboxing guidance for untrusted programs and models.

Exit criteria:

- Release cannot be published without SBOM, signatures, and security scan results.
- Model files and external SDKs are treated as untrusted inputs unless verified.

## Workstream 6: Developer experience

Goal: make the language practical for engineering teams.

Required deliverables:

- CLI help and examples.
- Project scaffolding.
- Formatter.
- Linter.
- VS Code syntax highlighting.
- Language Server Protocol roadmap.
- Clear tutorials for classifier, batch inference, RAG, model routing, and GreenAI report generation.

Exit criteria:

- A new developer can build, run examples, and interpret reports from a clean checkout.
- Error messages are understandable without reading compiler internals.

## Workstream 7: Enterprise deployment

Goal: provide a realistic runtime path for enterprise workloads.

Required deliverables:

- Container image build.
- Kubernetes/OpenShift manifests.
- Health checks.
- Structured logs.
- Prometheus metrics.
- Runtime configuration through environment variables.
- Model volume mounting.
- CI/CD example.
- Upgrade and rollback notes.

Exit criteria:

- A sample AI service can be built, containerized, deployed, monitored, and rolled back.

## Workstream 8: Governance and release model

Goal: establish predictable maintenance.

Required deliverables:

- RFC process for language changes.
- Maintainer responsibilities.
- Release cadence.
- Compatibility policy.
- Long-term support policy.
- Known limitations per release.
- Public claim review checklist.

Exit criteria:

- Every release has a signed checklist, evidence bundle, changelog, migration notes, and known limitations.

## Suggested milestones

### Milestone ER0: Historical baseline

Purpose: internal engineering review.

Required:

- Existing CI passes.
- Known limitations documented.
- Unsupported claims blocked.

### Milestone ER1: Compiler hardening

Purpose: stable internal pilots.

Required:

- Versioned grammar and language specification.
- Full positive/negative conformance suite.
- Sanitizer clean.
- Parser and semantic fuzzing added.

### Milestone ER2: Real AI backend pilot

Purpose: AI pilot with one real backend.

Required:

- ONNX Runtime CPU backend executes a model.
- Evidence distinguishes successful execution from fallback.
- Backend failure cases covered.

### Milestone ER3: Controlled enterprise beta (current)

Purpose: controlled enterprise pilot.

Required:

- Containerized runtime.
- Security scans.
- SBOM.
- Signed-publication workflow source; the protected tag exercise remains open.
- Telemetry and GreenAI evidence bundle.
- Deployment guide.

### Milestone ER4: General enterprise release candidate

Purpose: release candidate for external engineering teams.

Required:

- All mandatory scorecard items pass.
- Every backend/device/platform pair included in the declared production support set has live workload and numerical evidence.
- Compatibility policy active.
- Governance process active.
- Release evidence reviewed and retained.

## PR policy

Pull requests may improve the readiness score, but a PR must not be merged on the basis of ambition alone. Each PR must state which readiness items it satisfies and include tests. If a PR introduces public-readiness language, the language must be backed by passing gates and retained evidence.
