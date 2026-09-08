# C3-ECO assessment, eligibility, scoring and claims contract

Contract: `shorthand.c3eco.assessment.v1`

ShortHand PR90 adds a deterministic native C++17 assessment engine that consumes the typed PR88 C3-ECO profile, the instrument-backed PR89 measurement workbook and a bounded assessor scorecard. The engine produces machine-readable JSON plus a human-readable Markdown readiness report.

## Claim boundary

Assessment is not certification.

Every PR90 output carries `official_certification_granted:false` and `production_claim:false`. ShortHand does not act as a certification body, auditor, accreditation body or standards authority. The C3-ECO v0.6 material used by the repository is an authority-review draft for consultation. PR90 therefore computes certification-readiness evidence and permitted-claim candidates only. External authority review, retained auditor evidence and certification decisions remain separate.

PR90 also does not establish that ShortHand consumes less energy than Python. Equivalent-workload ShortHand-versus-Python measurement remains PR95.

## Inputs

`shorthand_c3eco_assess` accepts:

1. a compiler-generated `shorthand.c3eco.candidate_report.v1` with a conformant `shorthand.c3eco.profile.v2` typed profile,
2. an instrument-backed `shorthand.c3eco.measurement_workbook.v1`,
3. a tab-separated assessment scorecard,
4. a JSON output path, and
5. a Markdown output path.

The scorecard header is fixed:

```text
kind\tid\tvalue\tapplicable\tevidence_status\tevidence_ref
```

Inputs are bounded. Duplicate, unknown, malformed, incomplete and unsupported rows fail closed. The assessment must provide exactly G1-G14, the complete 76-criterion A-K catalog and the complete v1 control set.

## Mandatory eligibility gates

PR90 evaluates all fourteen C3-ECO draft-v0.6 gates. A failed gate invalidates certification readiness regardless of the numerical score.

| Gate | PR90 decision meaning |
| --- | --- |
| G1 | system identity is present and linked to the typed profile |
| G2 | functional unit is typed, positive and usable as the denominator |
| G3 | boundary declaration evidence passes |
| G4 | measured or otherwise accepted energy evidence passes; PR90 requires the PR89 instrumented workbook path |
| G5 | carbon calculation evidence passes and the PR89 workbook contains positive carbon accounting |
| G6 | security floor evidence passes |
| G7 | accessibility, safety and privacy floor evidence passes |
| G8 | repeatability evidence passes |
| G9 | retention evidence passes |
| G10 | requested public claims remain inside the supported assessment result |
| G11 | offsets and renewable instruments remain separate from the base footprint |
| G12 | surveillance and recertification acceptance evidence passes |
| G13 | quality and required-functionality guardrails pass |
| G14 | materiality passes, including cumulative omissions at or below 5 percent and no unaddressed individual omission at or above 1 percent |

The engine never converts a failed critical gate into a warning-only result.

## A-K scoring

The v1 engine uses the draft-v0.6 domain weights:

| Domain | Weight |
| --- | ---: |
| A Measurement integrity and carbon accounting | 12 |
| B Operational energy and runtime efficiency | 12 |
| C Compute, memory, storage and network efficiency | 12 |
| D Software stack, code and architecture efficiency | 12 |
| E Data lifecycle efficiency | 8 |
| F Cloud, infrastructure and deployment efficiency | 10 |
| G AI/ML and GenAI efficiency | 10 |
| H Hardware longevity and device impact | 6 |
| I Lifecycle, updates and maintainability | 6 |
| J User/admin autonomy and green defaults | 4 |
| K Governance, auditability and improvement | 8 |

The complete catalog is A1-A8, B1-B8, C1-C8, D1-D8, E1-E6, F1-F7, G1-G10, H1-H5, I1-I5, J1-J4 and K1-K7: 76 criteria in total. Every criterion is scored from 0 through 5. `insufficient` evidence caps the effective score at 1. `missing` evidence contributes 0. Strong evidence statuses are `documented`, `measured`, `verified` and `independent`.

Within a domain the applicable v1 criteria contribute equally to the domain percentage; the domain percentage is then multiplied by the fixed domain weight. Explicit penalty points are applied after weighting.

## Non-applicability

N/A is never a free score. Any N/A criterion requires an explicitly approved domain in `approved_na_domains` backed by verified or independent evidence.

A non-AI typed profile must make the whole G domain N/A. Its ten points are redistributed proportionally only across active A-F and K domains. An AI profile cannot remove the whole G domain. A fully N/A H or J domain redistributes only to active F/K. Other fully N/A domains redistribute across the remaining active domains. Total effective domain weight must remain exactly 100.

## Measurement quality, data quality and uncertainty

PR89 `high`, `medium` and `low` evidence maps to MQ/DQ 3, 2 and 1 for PR90. A scorecard may lower that class. Raising it to MQ4 or DQ4 requires `independent` evidence; an applicant cannot self-declare a stronger measurement class.

The engine applies conservative level ceilings:

| Readiness band | Score | Key evidence floors |
| --- | ---: | --- |
| Candidate | 40-49 | measured baseline only, not certification |
| Bronze | 50-64 | MQ1/DQ1, uncertainty <=30%, A>=40%, B>=35%, K>=40%, no applicable domain below 30% |
| Silver | 65-79 | MQ2/DQ2, uncertainty <=20%, A/B/K>=50%, eco-regression control |
| Gold | 80-89 | MQ3/DQ3, uncertainty <=12%, A/B/K>=65%, C/D>=60%, no high-severity claim risk |
| Platinum | 90-94 | MQ3/DQ3, uncertainty <=8%, A>=80%, B>=75%, K>=80%, independent review, public report and continuous telemetry |
| Diamond | 95-100 | MQ4/DQ4, uncertainty <=5%, A>=90%, B>=85%, K>=90%, independent review, public report, continuous telemetry, public evidence summary and registry readiness |

A numerical score may therefore be higher than the final candidate level. The output retains both the diagnostic score band and the candidate level after gates and caps.

## Eco-regression

If an equivalent prior-version energy baseline is supplied, PR90 compares it with current PR89 facility energy normalized by the typed functional-unit denominator.

An unexplained deterioration greater than 10 percent sets `corrective_action_required:true` and caps an otherwise eligible result at Bronze. An approved justification is explicit evidence; it is not inferred.

## Claim control

The assessment can evaluate candidate requests for measured baseline, Bronze, Silver, Gold, Platinum, Diamond and restricted claims. Certified-level wording is emitted only as `candidate_for_external_review`; ShortHand never emits an official certification decision.

`Green AI` requires an AI profile, supporting AI-boundary evidence and at least a Gold-level candidate result. Renewable-cloud hosting is supplementary only. Net-positive/regenerative is not a C3-ECO level. Zero-carbon and carbon-neutral wording remain restricted and subject to full-boundary evidence and external legal/technical review.

A comparative claim is deliberately rejected by the PR90 claims-integrity gate and returned as `deferred_pr95`. PR90 does not manufacture comparative evidence from compiler implementation choices or declared budgets.

## Determinism and safety

Assessment rows are stored by stable IDs and output is emitted in deterministic map order. Reordering the same input rows therefore produces byte-identical JSON and Markdown. Input files are bounded to 4 MiB and scorecard lines to 16 KiB. Duplicate keys/rows, forged certification flags, forged non-instrumented measurement sources and incomplete catalogs are negative qualification cases.

The implementation has no Python runtime dependency.

## Qualification

The first-class gate is `scripts/check_c3eco_assessment.sh`. It covers strict C++17 compilation, scoring-unit tests, real PR88 profile generation, real PR89 workbook generation, deterministic output, all-gate precedence, evidence-score caps, materiality, eco-regression, claim restrictions, forged upstream artifacts and malformed/incomplete scorecards. The same gate is registered in Make, CMake/CTest and sanitizer qualification.

PR91 remains responsible for signed/retained auditor bundles, replay, expiry and surveillance evidence. PR95 remains responsible for repeated equivalent-workload ShortHand-versus-Python performance and measured-energy qualification.
