# ShortHand production truth and C3-ECO traceability

production_truth_contract: shorthand.production.truth.v1
production_truth_source: docs/production_truth.tsv
c3eco_traceability_source: docs/c3eco_traceability.tsv
current_maturity: controlled_beta
production_claim: false

## Purpose

`docs/production_truth.tsv` is the machine-readable authority for the active maturity, roadmap, language-version, backend-support and certification-claim state. `docs/c3eco_traceability.tsv` records the implementation and verification state of every C3-ECO mandatory gate G1-G14, scoring domain A-K, and the S9/S12 software classes that directly apply to the ShortHand compiler and platform.

The gate `scripts/check_production_truth.sh` fails CI when an active control document contradicts these sources, a traceability row is missing, an evidence path is invalid, or a production blocker is represented as complete without retained verification evidence.

## Authority order

When documents disagree, use this order:

1. Executed source, tests, workflow results and retained artifacts establish what has actually passed.
2. `docs/production_truth.tsv` establishes the active maturity and roadmap state.
3. `docs/c3eco_traceability.tsv` establishes certification-readiness coverage and open blockers.
4. `docs/feature_implementation_status.md`, `docs/compiler_test_strategy.md` and `docs/production_readiness_pr_plan.md` explain the state for maintainers.
5. Historical plans and milestone anchors provide audit history only and cannot override active state.

Workflow source is not execution evidence. A declared backend is not live qualification. A source-level signing workflow is not a verified signed release. A candidate evidence bundle is not an external certification decision.

## Active language contract

The active language version is beta-0.3. It combines:

- the beta-0.2 base grammar and executable matrix,
- the beta-0.3 package, module, import and deterministic multi-file semantics,
- the `shorthand.c3eco.language.v1` certification-oriented declaration extension.

Only `int` and `bool` currently have the complete cross-mode executable core semantics documented in `docs/execution_semantics_beta_0_3.md`. Parser acceptance of `float`, `double` or `string` is not production execution support. PR84 owns the production type and memory model.

## C3-ECO profile boundary

The active certification-preparation profile uses the supplied C3-ECO Green Software Certification Standard draft v0.6 as the normative candidate and the 2026-07-18 v0.7 all-inclusive and eligibility documents as an inclusion and commercial-routing overlay. These documents remain consultation drafts. ShortHand may state that it produces C3-ECO-aligned candidate evidence, but it must not claim formal standards adoption, official certification, carbon neutrality, inherent language-level greenness or guaranteed financial savings.

The traceability matrix applies the stricter rule when the draft documents differ. Its `G1`-`G14` keys follow the v0.6 sequence. The `source` field records the v0.7 alias for each gate. Draft v0.7 inserts cost calculation where claimed at G6 and therefore renumbers its security, safeguard, repeatability, retention, claims and offset gates. PR83 maps that cost control into the canonical G5 carbon-calculation row, while preserving the v0.6 G6-G12 meanings. A cost or savings claim requires measured or transparently estimated kWh, a disclosed tariff, an explicit boundary and uncertainty; no fixed market-savings figure is a product claim.

Bronze is the first certification level. Registered and Measured Candidate are readiness states only. Efficiency evidence is invalid when functionality, accuracy, reliability, security, privacy, safety or accessibility is weakened. The framework evaluates the chosen implementation within a declared functional unit and boundary; it does not treat a programming language, model, cloud or framework as inherently green.

### Supplied certification source profile

PR83 reviewed the complete supplied Policy Club/C3-ECO document set and assigns each source a controlled role:

| Supplied source | Repository use |
| --- | --- |
| `C3-ECO_Green_Software_Certification_Standard_v0.6_updated.docx` | Normative candidate for mandatory gates, A-K domains, evidence quality, uncertainty, claims and audit controls. It remains an authority-review draft, not an adopted public standard. |
| `C3-ECO_All_Inclusive_Certification_Criteria.docx` and `C3-ECO_Certification_Eligibility_Criteria_and_Parameters.docx` | Dated v0.7 overlay for inclusive entry, Bronze-first certification, software classes, conditional cost calculation and commercial claim routing. |
| `C3-ECO_Final_Pitch.pdf`, `C3-ECO_Final_Pitch_One_Page.pdf` and `C3-ECO_Company_Certification_Pitch_One_Page.pdf` | Informative positioning only: useful-work measurement, engineering causes, full AI lifecycle, bounded claims and proposed-standard language. They cannot override gates or evidence requirements. |
| `C3-ECO_draft_annotated_review.pdf` and `C3-ECO_draft_annotated_casual_comments.pdf` | Informative reviewer controls: avoid causal claims that certification itself reduces emissions, remain technology-neutral, scale evidence by tier, broaden software scope, bound certificate claims and preserve quality/safety safeguards. |
| `C3-ECO_UK_Market_Savings_Figures.docx` | Informative economic examples only. Any use requires measured kWh, the disclosed dated tariff, scale assumptions and uncertainty. It is not a guaranteed ShortHand or customer savings claim. |

Where an informative pitch conflicts with a gate or claim restriction, the v0.6 gate and the stricter v0.7 overlay control. C3-ECO Certification may be described as operationalising a software-centric research/design framework into auditable candidate criteria; ShortHand cannot describe the draft as an EU, ISO, IEC, Commission-approved, harmonised or legally required standard.

## Current declared production scope

The only production-qualified backend/device scope is `linux-x64-cpu-v1`, with ONNX Runtime CPU live numerical execution. GPU, TPU and NPU discovery remains inventory evidence only. Any expansion requires its own live device-backed numerical qualification.

ShortHand remains `controlled_beta` with `production_claim: false` until every production blocker in the coverage and traceability matrices closes, the final PR96 release-candidate aggregate is green, and the protected release exercise succeeds.

## Change control

Any change to maturity, production claims, language version, remaining PR count, backend support, C3-ECO profile, certification claim status, release status, mandatory skip policy, gate state, domain state or closure target must update both the relevant TSV source and its executable guard in the same PR.
