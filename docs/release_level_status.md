# Release Level Status

release_level_status_version: 2026-09-22-pr103
current_maturity: controlled_beta
production_claim: false
current_github_pr: 103
last_merged_github_pr: 102
final_planned_github_pr: 102
final_planned_roadmap_pr: 102

ShortHand is a controlled beta with a versioned `linux-x64-cpu-v1` backend qualification scope. It is not an enterprise production release, an external certification, or a general accelerator-support claim.

## Implemented release infrastructure

- deterministic multi-toolchain builds and installed consumers,
- frozen runtime ABI 1.0.0,
- mandatory sanitizer, fuzz and race gates,
- CodeQL, dependency, secret and license policy gates,
- hardened amd64/arm64 container and live Kubernetes qualification,
- SPDX 2.3 source and release-artifact SBOM generation,
- candidate provenance, signing workflow and rollback logic,
- runtime JSON, Prometheus and OTLP-shaped observability exports,
- bounded process-scoped serving with deadlines, cooperative cancellation, backpressure, tenant isolation, health and graceful drain,
- live ONNX Runtime CPU numerical qualification for `linux-x64-cpu-v1`,
- machine-readable production truth and C3-ECO traceability.
- beta-0.4 exact float/string/typed-array execution and guarded type/memory descriptors.
- beta-0.5 expression calls, recursion, lexical cleanup, structured returns and safe label resolution.
- beta-0.6 enterprise ABI schemas, SHA-256 offline packages, SPDX dependency evidence and core FFI ABI 1.0.0.
- beta-0.7 typed C3-ECO profile links, deterministic migration and claim-safe preparation evidence.
- instrument-backed C3-ECO energy/carbon/cost accounting with provenance, uncertainty and deterministic reconciliation.
- candidate-only C3-ECO eligibility, complete A-K scoring, claim controls and quality/eco-regression assessment.

## Open before enterprise production use

Roadmap PR94-PR96 qualify representative workloads, measured performance/energy and a scoped enterprise RC. PR97-PR99 add the merged audit's broader release evidence; PR99 implements the fail-closed pilot/RC aggregate and leaves its decision blocked by retained evidence. TST017 separately requires a real protected tag publication whose attestations verify cryptographically. Public ingress, authentication, authorization and TLS are not claimed by the process-scoped serving contract.

Only executed checks and retained artifacts are release evidence. Workflow source, a detected device, an installed SDK, a skipped test or a candidate evidence bundle is not proof of production readiness.

Historical release marker: release_level_status_version: 2026-09-01-pr88.

PR91 implements `shorthand.c3eco.auditor_bundle.v1`: signed artifact/reference lineage, native assessment replay, retention-policy and surveillance verification, nonconformity handling, redacted public reports and separate estimated readiness. See [the auditor contract](c3eco_auditor_bundle.md). It does not grant certification or independently verify storage retention.

PR92 provides the generated MLIR dialect and SDK for Linux x64 with LLVM/MLIR 18, with mandatory lit, verifiers, sanitizer and installed-consumer evidence. GitHub PR94 adds source/runtime lowering in this scope; other MLIR platforms remain unqualified.

GitHub PR94 implements original roadmap PR93 in the Linux x64/LLVM18 scope: verified SemanticIR, source and SDK lowering, bounded composite values, checked real ONNX runtime calls and optimization-preserved evidence. See [the lowering contract](mlir_lowering.md). TST024 is implemented within this scope. Merged GitHub PR93 is the separate gap assessment; PR101 and PR102 are merged; PR102 closes the planned implementation sequence. Physical observations, full family baselines, independently reviewed organizational operations and the protected release exercise remain mandatory external evidence.

PR99 adds [the enterprise pilot and release-candidate contract](enterprise_pilot_release_candidate.md) for the explicit `linux-x64-cpu-v1` support set. A candidate report records clean install/reinstall/uninstall, rollback, serving soak, deployment contract and all retained blockers; it does not claim enterprise GA, accelerator support or certification.

Historical release marker: release_level_status_version: 2026-09-15-pr97.

PR101 implements [independent reproduction and draft-standard pilot verification](c3eco_independent_pilot.md) over signed PR91 bundles. Candidate consistency receipts do not authenticate physical measurements, legal independence, actual storage retention or certification. G8/S9/S12 and TST017/TST025/TST026 remain partial; PR102 is merged and the planned implementation sequence is closed.

PR102 adds [release claims and audit closeout](release_claims_closeout.md). Its 25-finding ledger retains 13 evidence-pending findings; public capabilities and live release identity are checked without granting GA authority. Physical/quality/organizational/protected-release evidence remains mandatory after the implementation sequence.

PR103 is a governance-only closeout and registers [the post-PR102 enterprise GA and C3-ECO evidence plan](post_pr102_enterprise_c3eco_release_plan.md). The implementation roadmap is closed; the evidence roadmap remains active. GA publication, official certification, comparative-energy and lowest-carbon claims remain blocked.
