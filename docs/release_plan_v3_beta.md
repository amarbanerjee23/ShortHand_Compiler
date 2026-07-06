# Release Plan v3 Beta

This release plan defines the v3 beta path for ShortHand. It turns the enterprise readiness strategy into ordered beta work items and acceptance evidence.

## Beta objective

The v3 beta objective is not to declare full industrial production readiness. The objective is to make ShortHand suitable for controlled enterprise pilots by tightening documentation, validation expectations, evidence boundaries, and release discipline.

## Scope of this PR

This PR fulfills the documentation and governance requirements needed to define a beta candidate:

1. Enterprise beta requirements are defined in `docs/beta_enterprise_requirements.md`.
2. The v3 beta release plan is defined in this file.
3. Existing CI remains the build authority.
4. Existing enterprise scorecard remains the source of maturity truth.
5. The current maturity remains ER0 until CI, backend, security, deployment, and governance gaps are closed.

## One-by-one fulfillment plan

### Step 1: Define beta requirements

Requirement:

- Create a clear list of beta-level enterprise requirements.

Fulfillment:

- Added `docs/beta_enterprise_requirements.md`.
- Requirements are grouped into language contract, compiler validation, AI runtime behavior, GreenAI evidence, security baseline, developer experience, deployment, and governance.

Evidence:

- Requirements file exists and is part of the PR.

### Step 2: Keep maturity honest

Requirement:

- Do not overstate current maturity.

Fulfillment:

- This plan explicitly states that v3 is a beta path, not a full industrial release.
- The enterprise scorecard remains the maturity authority.

Evidence:

- `docs/enterprise_release_scorecard.md` continues to list ER0 as the current state until gates are complete.

### Step 3: Preserve build authority

Requirement:

- CI remains the source of truth for merge readiness.

Fulfillment:

- This PR avoids high-risk CI rewiring.
- Existing CI still runs setup, strict validation, smoke tests, Makefile tests, sanitizer tests, CMake build, and CTest.

Evidence:

- Latest PR head must have successful GitHub Actions CI before merge.

### Step 4: Preserve AI runtime honesty

Requirement:

- Do not claim real inference when fallback is used.

Fulfillment:

- Beta requirements explicitly require fallback and real execution to be separated.
- Real backend execution remains an open post-beta requirement until implemented and evidenced.

Evidence:

- Existing known limitations and README fallback wording remain unchanged.

### Step 5: Preserve GreenAI evidence honesty

Requirement:

- Do not turn evidence reports into certification claims.

Fulfillment:

- Beta requirements keep GreenAI and C3-ECO-aligned reports as evidence-only artifacts.
- Full external certification remains outside this repository unless reviewed by an external authority or auditor.

Evidence:

- Existing README and GreenAI docs keep the evidence-only disclaimer.

### Step 6: Define the next implementation backlog

Requirement:

- Identify what still needs to be built for a real enterprise release candidate.

Fulfillment:

- This plan carries forward the open implementation backlog.

Open backlog:

1. ONNX Runtime CPU backend execution.
2. Backend failure and compatibility matrix.
3. Parser and semantic fuzzing.
4. Formal compatibility policy.
5. SBOM and release signing.
6. Container and Kubernetes/OpenShift deployment examples.
7. Runtime observability hooks.
8. Full evidence bundle generator.
9. Editor tooling and language server roadmap.

## v3 beta merge criteria

This PR may be merged only if:

1. It is mergeable with no conflicts.
2. CI passes on the latest head SHA.
3. The change does not remove any existing validation gate.
4. The change does not remove any known limitation.
5. The change does not introduce unsupported readiness or certification claims.

## v3 beta non-goals

This PR does not claim to complete:

- Real AI backend execution.
- Full enterprise deployment support.
- External certification.
- SBOM and signed release artifacts.
- Formal long-term support.
- Complete developer tooling.

## Release decision

The correct release decision after this PR is:

- Accept the v3 beta requirements and plan if CI passes.
- Keep the project maturity label conservative.
- Start implementation PRs for real backend execution, security release artifacts, deployment examples, and observability.

## Next PR sequence

Recommended next PRs:

1. `beta-onxxruntime-cpu-backend`: implement and test real ONNX Runtime CPU execution.
2. `beta-diagnostics-matrix`: add grammar and semantic diagnostics matrix.
3. `beta-security-baseline`: add security policy, SBOM workflow, and release signing plan.
4. `beta-container-runtime`: add container and OpenShift/Kubernetes examples.
5. `beta-observability`: add structured logs and runtime metrics plan.
