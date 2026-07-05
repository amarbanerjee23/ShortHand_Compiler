# ShortHand Beta Enterprise Requirements

This document defines the beta-level enterprise requirements for ShortHand. It is written as an implementation checklist for moving the language from internal engineering review toward controlled enterprise pilots.

## Release positioning

Release target: Beta candidate for controlled enterprise pilots.

Current claim boundary:

- ShortHand may be described as a beta candidate only after CI passes and the release notes identify remaining gaps.
- ShortHand must not be described as fully enterprise-ready until all enterprise release scorecard gates are satisfied with retained evidence.
- GreenAI and C3-ECO-aligned reports remain evidence artifacts only unless reviewed by an external authority or auditor.

## Requirement groups

### R1. Language contract

Purpose: make the syntax and semantics stable enough for pilot users.

Beta requirements:

1. Define the supported beta syntax surface.
2. Identify unstable or experimental syntax.
3. Keep model, tensor, inference, GreenAI contract, and GreenAI measurement syntax documented.
4. Require tests for each supported beta syntax feature.
5. Add a compatibility note for any breaking grammar change.

Beta fulfillment status:

- Supported language constructs are documented in `docs/language_spec.md`.
- The beta release plan requires a future grammar matrix before moving beyond beta.
- Breaking syntax must be documented before adoption.

### R2. Compiler build and validation

Purpose: make clean checkout validation reliable.

Beta requirements:

1. `bash setup_build_infra.sh` must pass.
2. `bash scripts/validate_language.sh --strict` must pass.
3. `bash scripts/smoke_test.sh` must pass.
4. `make -C Compiler_new_ws/Short_Hand/src test` must pass.
5. `make -C Compiler_new_ws/Short_Hand/src sanitize` must pass.
6. CMake configure, build, and CTest must pass.

Beta fulfillment status:

- CI already runs these gates.
- PRs must not be merged when these gates fail.

### R3. AI runtime behavior

Purpose: avoid misleading AI execution claims.

Beta requirements:

1. Runtime evidence must distinguish real execution from fallback.
2. Fallback must not report successful inference.
3. Missing SDKs must be visible to the user.
4. Backend preference ordering must be preserved.
5. Invalid model declarations must fail with controlled diagnostics.

Beta fulfillment status:

- Deterministic fallback behavior is documented.
- Real ONNX Runtime CPU execution remains a post-beta requirement unless added with optional SDK evidence.

### R4. GreenAI and C3-ECO-aligned evidence

Purpose: keep sustainability evidence auditable and honest.

Beta requirements:

1. Reports must preserve the evidence-only disclaimer.
2. Reports must not fabricate energy, carbon, accuracy, latency, or runtime values.
3. Missing measurement tools must be reported as unavailable.
4. Offsets and avoided impact must not reduce base footprint values.
5. Functional unit and boundary concepts must remain visible in examples and docs.

Beta fulfillment status:

- Evidence-only policy exists in README and GreenAI docs.
- C3-ECO-aligned scope remains evidence-level, not certification-level.

### R5. Security and supply-chain baseline

Purpose: define the minimum needed before an enterprise pilot.

Beta requirements:

1. Document unsupported external SDKs and model-file assumptions.
2. Do not vendor large third-party SDK binaries.
3. Preserve deterministic behavior when optional dependencies are absent.
4. Add a future requirement for SBOM, signed artifacts, and vulnerability disclosure before any release candidate beyond beta.

Beta fulfillment status:

- External SDKs are optional and not vendored.
- SBOM and signing remain open release-candidate requirements.

### R6. Developer experience

Purpose: make pilot use practical.

Beta requirements:

1. A clean checkout must include build instructions.
2. Examples must show AI abstraction and GreenAI evidence flows.
3. Documentation must identify known limitations.
4. User-facing diagnostics should be clear enough for pilot users.

Beta fulfillment status:

- README, language spec, known limitations, and readiness docs provide pilot-level guidance.
- Formatter, linter, and editor integration remain post-beta requirements.

### R7. Deployment and operations

Purpose: prepare the language for enterprise deployment without overstating readiness.

Beta requirements:

1. Define container and Kubernetes/OpenShift work as future release-candidate requirements.
2. Do not claim deployable enterprise runtime status until examples and runtime images exist.
3. Track deployment readiness in the enterprise scorecard.

Beta fulfillment status:

- Deployment work is documented as open in the scorecard.

### R8. Governance and release control

Purpose: make adoption predictable.

Beta requirements:

1. Keep a release plan.
2. Keep a scorecard.
3. Maintain known limitations.
4. Avoid unsupported public claims.
5. Require CI pass before merge.

Beta fulfillment status:

- Enterprise strategy and scorecard exist.
- This beta requirements document adds the next layer of detail.

## Beta acceptance checklist

A beta PR is acceptable only when:

- It is mergeable with no conflicts.
- CI passes on the latest head SHA.
- It does not weaken existing evidence, fallback, or claims controls.
- It keeps current maturity honest.
- It does not describe the project as fully enterprise-ready.

## Requirements not yet fulfilled for a full enterprise release

The following remain open after beta:

1. Real ONNX Runtime CPU backend execution.
2. Backend execution test matrix.
3. SBOM and signed artifacts.
4. Security disclosure process.
5. Container image and Kubernetes/OpenShift deployment examples.
6. Formal language compatibility policy.
7. Formatter, linter, and editor tooling.
8. Runtime observability hooks.
9. Full evidence bundle generation.
10. External audit or certification path for any external GreenAI claim.
