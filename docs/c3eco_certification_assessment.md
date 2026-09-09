# C3-ECO eligibility, scoring, claims and eco-regression assessment

c3eco_assessment_contract: shorthand.c3eco.assessment.v1
decision_kind: candidate_recommendation_only
normative_candidate: C3-ECO draft v0.6
production_claim: false
official_certification_granted: false

## Purpose

PR90 adds a deterministic native assessment engine after the typed profile and measured-accounting contracts. `shorthand_c3eco_assess` validates a conformant `shorthand.c3eco.profile.v2` candidate report and an instrumented `shorthand.c3eco.measurement_workbook.v1`, then evaluates mandatory gates, weighted criteria, materiality, AI responsibilities, requested claims and regressions.

The result is a candidate recommendation for external review. It is not a certificate, auditor signature, public certification report or permission to claim a C3-ECO level. Every output fixes `official_certification_granted:false`, `production_claim:false`, `comparative_energy_claim:false` and `level_claim_permitted:false`. It also retains the MQ, DQ, uncertainty, review, telemetry, device-impact and claim-risk controls used for the recommendation. PR91 owns signed auditor bundles, retention, expiry, recertification and public reporting.

## Candidate directory contract

The command is:

```text
shorthand_c3eco_assess <candidate-directory> <assessment-output.json> [report.md]
```

The candidate directory contains exactly named contract inputs:

| File | Required content |
| --- | --- |
| `profile.json` | Structurally complete `shorthand.c3eco.candidate_report.v1` carrying exactly one complete typed `shorthand.c3eco.profile.v2`, no migration requirement and no enabled certification or production claim. |
| `measurement.json` | Complete instrumented `shorthand.c3eco.measurement_workbook.v1` with record-level derivations and aggregate energy, carbon, uncertainty and cost reconciliation, offsets excluded from base-footprint reduction and no certification claim. |
| `metadata.tsv` | Linked certification identity/version, class, functional-unit, boundary and workload identifiers, MQ/DQ, uncertainty, penalty points, AI route, claim-risk and review-control values. The fixed header is `key`, `value`, `evidence_status`, `evidence_ref` separated by tabs. |
| `gates.tsv` | Exactly one evidenced `pass` or `fail` row for every mandatory gate G1 through G14. |
| `criteria.tsv` | Exactly the 76 catalog criteria A1-A8, B1-B8, C1-C8, D1-D8, E1-E6, F1-F7, G1-G10, H1-H5, I1-I5, J1-J4 and K1-K7, with scores, weights, evidence sufficiency, applicability and auditor approval for every N/A decision. |
| `materiality.tsv` | A complete 100 percent component allocation using `included`, `conservative_estimate` or `omitted` dispositions. |
| `claims.tsv` | Requested claim type, exact text, scope/equivalence controls and legal/technical evidence status. |
| `regressions.tsv` | Eco and quality baseline/current values, direction, explanation, corrective action and evidence. |

TSV headers, complete catalog membership, row cardinality, canonical identifiers, closed enums, numeric ranges, duplicate identifiers and required evidence references are fail-closed. Invented or omitted criteria are rejected. JSON and TSV inputs are size-bounded, TSV rows and lines are bounded, and non-finite derived arithmetic is rejected. The embedded JSON reader rejects malformed JSON, duplicate object keys and excessive nesting instead of accepting text-pattern approximations. It accepts valid UTF-16 surrogate pairs in escaped JSON strings and rejects invalid pairs.

## Eligibility precedence

All G1-G14 gates are evaluated before level selection. A failed gate yields `not_eligible` regardless of the calculated score. Derived controls can make a submitted gate fail:

- any unsupported requested claim fails G10;

- any observed quality deterioration fails G13;
- an individually omitted component at or above 1 percent fails G14;
- cumulative omitted footprint above 5 percent fails G14.

A conservative estimate is an addressed component, not an omission. An input `pass` remains visible beside the effective gate status so a derived failure cannot be hidden.

## A-K scoring

The fixed domain weights sum to 100:

| Domain | Weight | Domain | Weight |
| --- | ---: | --- | ---: |
| A | 12 | G | 10 |
| B | 12 | H | 6 |
| C | 12 | I | 6 |
| D | 12 | J | 4 |
| E | 8 | K | 8 |
| F | 10 |  |  |

For domain $d$:

$$
P_d = 100 \times \frac{\sum_i(s_i w_i)}{5\sum_i w_i}, \qquad
S = \sum_d W'_d \frac{P_d}{100}
$$

where $s_i$ is the effective 0-5 criterion score and $w_i$ is its criterion weight. Insufficient evidence caps $s_i$ at 1 even if a higher score was entered. Applicability is all-or-nothing for a domain, which prevents criterion cherry-picking. Every row in an N/A domain requires an auditor approval reference, after which the fixed domain weight is reallocated proportionally across applicable domains. A, B and K cannot be N/A. G cannot be N/A when AI is in scope, and H cannot be N/A for material client-device impact. Because Gold explicitly requires C and D minima, an approved N/A for either domain limits the recommendation to Silver.

## Level recommendation rules

Higher levels inherit the lower-level safeguards. The score creates only a ceiling; prerequisites and caps can lower the recommendation. Evidenced penalty points in [0,100] are deducted after weighting, with a floor of zero.

MQ and DQ start from the weakest workbook record (`low` = 1, `medium` = 2, `high` = 3). The metadata may lower them. Raising either class requires independent review plus `evidence_status=independent` for that quality control, including MQ4/DQ4. Uncertainty uses the greater of the declared percentage and every measured record percentage; an optimistic metadata value cannot hide measured uncertainty. These are evidence-review controls, not cryptographic proof of the referenced evidence; PR91 owns signed lineage.

Identity, version, software class and scope identifiers must match the linked typed profile. All six profile declaration links must resolve, and a class-S6 profile cannot disable AI scope. Metadata accepts the typed profile aliases `S6_AI_GENAI`, `S9_DEVELOPER_TOOLS_CI_CD` and `S12_INFRASTRUCTURE_PLATFORM` as well as S1-S12.

| Recommendation | Score and required controls |
| --- | --- |
| Candidate | Score at least 40 and all mandatory gates pass. This is measured candidate status, not certification. |
| Bronze | Score at least 50; A at least 40%, B at least 35%, K at least 40%; every applicable domain at least 30%; uncertainty at most 30%. |
| Silver | Score at least 65; A/B/K at least 50%; MQ2/DQ2 or better; eco-regression control defined; uncertainty at most 20%. |
| Gold | Score at least 80; A/B/K at least 65%; C/D at least 60%; MQ3/DQ3 or better; no high-severity claim risk; uncertainty at most 12%; AI candidates compare at least two quality-energy options. |
| Platinum | Score at least 90; A at least 80%, B at least 75%, K at least 80%; independent review, public report and continuous telemetry; uncertainty at most 8%. |
| Diamond | Score at least 95; A at least 90%, B at least 85%, K at least 90%; MQ4/DQ4; continuous telemetry and public evidence; uncertainty at most 5%. |

`recommended_level_for_external_review` never means that the level was awarded. Only a qualified external authority can issue or publish a certification decision.

## AI route

AI scope requires one explicit role: `model_provider`, `application_deployer`, `integrator` or `auditor`. It also requires training and inference boundaries, including an explicit training exclusion when training is outside applicant control, a token or inference functional metric and declared exclusions. Gold or higher requires at least two measured quality-energy frontier options. Non-AI candidates use `ai_role=not_applicable`; approved domain-G N/A treatment is then possible.

## Claim control

The engine evaluates each claim request, then reevaluates eligibility if a request fails G10:

- `candidate_assessment` may be permitted only for the exact assessed scope;
- `certified` and `level` are always blocked because external authority is required;
- `offsets_only` is always blocked because offsets cannot replace base-footprint reduction;
- `comparative` and `best` remain blocked with `comparative_qualification_deferred_pr95`, even when equivalence controls are supplied;
- `zero`, `climate_positive` and `net_positive` require separate legal/technical evidence and external review; PR90 cannot authorize them;
- `green_ai` requires at least Gold candidate evidence plus the AI role, lifecycle boundaries, token/inference metric and exclusions;
- high-severity claim risk blocks every non-candidate claim.

A permitted request means the supplied control conditions are present. A blocked request invalidates G10 and therefore the candidate level, while the diagnostic score remains visible. It is not external certification, legal advice or proof that the repository already has the evidence. Candidate-assessment text is separately checked against certification-level, superiority, neutrality, offset and superlative wording so a dangerous claim cannot be relabelled as a candidate statement. ShortHand currently sets `comparative_energy_claim:false`; PR95 owns repeated equivalent-workload ShortHand-versus-Python evidence.

## Eco-regression and surveillance

Deterioration is calculated relative to a positive baseline with an explicit better-direction flag. More than 10 percent eco-regression triggers corrective action and recertification review. If either the explanation or corrective-action reference is absent, the external-review recommendation is capped at Bronze. Any quality deterioration fails G13 and cannot be compensated by an efficiency score.

Surveillance cadence is six months for AI or high-scale SaaS candidates and twelve months otherwise. Architecture, model, runtime, database, cloud region, provider, hardware and traffic changes are listed as recertification-review triggers.

## Qualification

The optional Markdown report renders the same decision, domain scores, MQ/DQ, penalties, claim reasons and surveillance cadence as JSON, with escaped input text. Both outputs are deterministic and checked for write errors.

`scripts/check_c3eco_assessment.sh` generates real PR88 typed profiles and PR89 instrumented workbooks, executes independent native scoring tests, and covers the complete 76-criterion catalog, exact tier boundaries, mandatory-gate precedence, evidence score caps, N/A weight reallocation, AI/device N/A restrictions, optional-domain level caps, materiality completeness and thresholds, claim restrictions, uncertainty/MQ/DQ/control downgrades and penalty deductions, eco and quality regressions, deterministic input ordering, malformed JSON, upstream contract spoofing, record and aggregate reconciliation, missing/duplicate records and range errors. The gate runs directly in CI and through Make, sanitizers and CTest. Both native evidence tools are installed by the production packaging gate.

The earlier draft five-path scorecard CLI is superseded by this candidate-directory contract before PR90 is merged. The consolidated schema and documentation are authoritative; no duplicate legacy engine is installed.
