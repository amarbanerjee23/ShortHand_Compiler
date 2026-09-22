# Post-PR102 enterprise GA and C3-ECO evidence plan

post_pr102_plan_version: 2026-09-22-pr103
plan_status: active
implementation_roadmap_status: closed_at_pr102
governance_closeout_pr: 103
current_maturity: controlled_beta
production_claim: false
ga_publication_authorized: false
official_certification_granted: false
comparative_energy_claim: false
lowest_carbon_language_claim: false
production_scope: linux-x64-cpu-v1

## Purpose

PR102 completed the planned compiler/runtime implementation sequence. The remaining work is not a reason to invent more feature PRs: it is a bounded evidence and operations program that must prove enterprise release readiness and any sustainability claim with retained, independently reviewable evidence.

This plan closes three distinct gaps:

1. correct stale repository bookkeeping after the PR102 merge;
2. obtain the operational, quality, security and release evidence required for an enterprise GA decision;
3. obtain calibrated, quality-equivalent, independently reproducible evidence before any C3-ECO certification or lowest-carbon claim is allowed.

A failed evidence milestone may create a remediation PR, but no remediation PR is pre-authorized and no failed or unavailable check may be converted into a skip, warning-only success or narrower test merely to reach GA.

## Non-negotiable invariants

- The active maturity remains `controlled_beta` until the final GA gate succeeds.
- `production_claim:false`, `ga_publication_authorized:false`, `official_certification_granted:false`, `comparative_energy_claim:false` and `lowest_carbon_language_claim:false` remain fail-closed defaults.
- The qualified backend scope remains `linux-x64-cpu-v1`; GPU, TPU and NPU inventory does not establish production execution.
- Every comparison must preserve functional output, accuracy/quality, reliability, security, privacy, safety and accessibility requirements.
- Synthetic fixtures, replayed traces and candidate receipts may validate tooling but cannot substitute for required physical measurements or independent organizational operation.
- Existing sanitizer, race, fuzz, security, portability, Kubernetes, live-ONNX, reproducibility, CTest and zero-skip gates remain mandatory.
- Claims must be bounded to the measured workload, hardware, software revision, compiler flags, model, dataset, functional unit and measurement boundary.

## PR103 — governance closeout

PR103 is a governance-only closeout. It must not change ShortHand language syntax, compiler semantics, runtime ABI or the PR95 physical-measurement contract.

Required changes:

- set PR102 as the final merged planned implementation PR;
- set planned implementation increments to zero;
- mark the implementation roadmap closed at PR102 while keeping the evidence program active;
- make the production-truth and roadmap gates reject a regression back to the stale PR101/PR102-in-progress state;
- register this post-implementation evidence plan as the authority for GA and sustainability claim progression;
- preserve every evidence-pending finding in the PR102 closeout ledger.

Exit: both exact-head `ci / ubuntu (push)` and `ci / ubuntu (pull_request)` are green, with no mandatory job weakened or skipped.

## Evidence milestones

| Milestone | Scope | Findings / tests primarily closed | Required evidence | Exit condition |
| --- | --- | --- | --- | --- |
| E1 — protected release and repository controls | Prove the real publication path rather than only its source contract. | SH-EA-006, SH-EA-007, TST017 | protected `master`/ruleset review and status controls, protected `production-release` environment, real RC/version tag, signed artifacts, SBOM/provenance, verifiable attestations, rollback/restore exercise and retained logs | real protected publication succeeds; attestations verify cryptographically; rollback is demonstrated; no required control is bypassed |
| E2 — representative enterprise AI workload and quality evidence | Replace bounded/synthetic family evidence with representative end-to-end workloads and quality-equivalent baselines. | SH-EA-002, SH-EA-011, SH-EA-020, TST025 | retained standard or otherwise defensible datasets/models for classification, retrieval, detection, batching/concurrency, quantized inference and material training/lifecycle paths; preprocessing/inference/postprocessing/serving traces; accuracy/quality, latency, reliability and failure evidence | each in-scope family has a real workload, reproducible quality result and equivalent baseline; no energy optimization degrades the quality/safety floor |
| E3 — calibrated physical energy and eco-regression | Produce measurement-grade energy evidence coupled to the exact workloads from E2. | SH-EA-003, SH-EA-018, TST026 | calibrated CPU energy and, where applicable, external/platform power evidence; raw time-series readings; warm-up policy; repeated paired trials; clock/calibration metadata; uncertainty; failed-run retention; exact binary/model/dataset hashes; optimized Python and native C++/runtime baselines | repeated equivalent-workload measurements are statistically and operationally reviewable; uncertainty is reported; eco-regression policy can reject an adverse release |
| E4 — full lifecycle, data, carbon and hardware boundary | Close material impacts that execution-only measurements do not cover. | SH-EA-012, SH-EA-013, SH-EA-015 | data movement/storage/retention observations, cloud allocation and factors with provenance, network/egress where material, embodied/lifetime allocation, older/minimum hardware qualification and payload/update regressions | all material boundary items are either measured with provenance or explicitly justified as immaterial; no double counting |
| E5 — independent reproduction and C3-ECO operations | Move from candidate self-verification to independently operated and retained evidence. | SH-EA-005, SH-EA-024, SH-EA-025, G8, S9, S12 | independent organization/lab reruns, separately controlled environment and keys, retained signed bundles, restore/access tests, surveillance/recertification operation, nonconformity handling and independent claim review | independent reruns reproduce the bounded findings; retention/surveillance is actually operated; any certification decision is issued externally rather than inferred by the compiler |
| E6 — enterprise GA decision | Aggregate the final release candidate only after E1–E5 are complete. | production RC decision and all remaining production blockers | clean install/reinstall/uninstall, upgrade/rollback, representative pilot, soak/fault/recovery evidence, support/incident/release documentation, exact-head mandatory CI, signed release evidence and final blocker report | zero unresolved production blockers for the declared scope and a fail-closed RC report explicitly permits GA publication |

## Enterprise production release rule

Enterprise GA is a decision over a declared scope, not a synonym for “all tests pass.” For the initial release the scope remains `linux-x64-cpu-v1`.

The GA gate must reject release when any of the following is unresolved:

- a mandatory CI context is not successful on the reviewed head;
- TST017, TST025 or TST026 is incomplete;
- any PR102 `evidence_pending` S0 finding remains unresolved;
- the release artifact cannot be reproduced and its signature/attestation cannot be verified;
- the production pilot/RC report remains `blocked_by_open_evidence`;
- quality, reliability, security, privacy, safety or accessibility is worse than the accepted baseline;
- a required physical measurement is represented only by a synthetic fixture, replay or declared estimate.

The current process-scoped serving runtime does not claim public ingress, TLS, authentication or authorization. A GA release must either keep that exclusion explicit or add and qualify those capabilities in a separate remediation PR before claiming them.

## C3-ECO and carbon-claim ladder

Claims progress only in this order:

| Level | Permitted wording | Minimum evidence |
| --- | --- | --- |
| C0 — current | controlled-beta implementation; C3-ECO-aligned candidate evidence for the declared scope | current merged implementation and claim-safe candidate tooling |
| C1 — measured workload | a named ShortHand workload used a measured amount of energy/carbon under a stated boundary | E2 + E3 physical evidence with quality equivalence |
| C2 — comparative workload | ShortHand measured lower energy/carbon than a named, optimized baseline for the same functional unit on the stated hardware | E2 + E3, paired/repeated measurements, uncertainty and no quality degradation |
| C3 — independently reproduced comparative result | the C2 result was reproduced by an independent party under the stated protocol | E5 independent rerun and signed evidence |
| C4 — C3-ECO certification claim | only the exact level/scope granted by the external certification authority | external certification decision plus operated retention/surveillance controls |
| C5 — lowest-carbon claim | only a tightly bounded statement over a pre-declared comparison universe, workloads, hardware and period | independent benchmark study covering representative alternatives, quality-equivalent workloads, calibrated lifecycle-aware measurements, statistical uncertainty, conflict-of-interest disclosure and independent review |

A global statement such as “ShortHand is the lowest-carbon programming language” is not authorized by C1–C4. Even at C5 the wording must identify the evaluated universe and conditions unless a defensible independent study genuinely supports a broader statement.

## Lowest-carbon benchmark protocol

Before C5 can be considered, freeze a public benchmark protocol that includes:

- the exact candidate ShortHand revision and release artifact;
- representative AI application families and real datasets/models;
- at least optimized Python/framework and native C++ runtime baselines, plus other materially relevant production alternatives selected before results are observed;
- identical hardware, model semantics, data, warm-up, thread/affinity settings, accuracy targets and functional outputs;
- compiler/runtime versions and optimization flags;
- calibrated workload-coupled power/energy collection and lifecycle boundary accounting;
- repeated randomized or counter-balanced trials with raw traces and failed runs retained;
- uncertainty intervals and effect sizes, not only point estimates;
- independent reproduction on separately controlled hardware;
- a public report that publishes unfavorable as well as favorable results.

If ShortHand is not best on a workload, the result is retained and the claim is narrowed rather than the workload being removed.

## Mapping of the PR102 evidence-pending ledger

| Finding | Milestone |
| --- | --- |
| SH-EA-002 | E2 |
| SH-EA-003 | E3 |
| SH-EA-005 | E5 |
| SH-EA-006 | E1 / E6 |
| SH-EA-007 | E1 |
| SH-EA-011 | E2 |
| SH-EA-012 | E4 |
| SH-EA-013 | E4 |
| SH-EA-015 | E4 |
| SH-EA-018 | E3 |
| SH-EA-020 | E2 |
| SH-EA-024 | E5 |
| SH-EA-025 | E5 |

## Remediation-PR rule

The evidence program does not allocate artificial PR numbers after PR103. A new implementation PR is opened only when an evidence milestone exposes a concrete defect or missing production capability. That PR must:

1. name the failed evidence milestone and finding;
2. implement the smallest corrective change;
3. add a regression that fails before and passes after the correction;
4. preserve all existing mandatory gates;
5. rerun the affected physical/operational evidence after merge;
6. never mark the evidence milestone complete merely because the remediation code merged.

## Final release decision

PR103 closes bookkeeping, not GA. After PR103, the implementation roadmap is closed and the evidence roadmap is active. GA publication and sustainability claims remain blocked until their corresponding evidence milestones have actually succeeded and the retained evidence is reviewable.
