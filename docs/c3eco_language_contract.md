# ShortHand C3-ECO first-class language contract

c3eco_language_contract_version: shorthand.c3eco.language.v1
production_claim: false
official_certification_granted: false
normative_candidate: C3-ECO draft v0.6
inclusion_overlay: C3-ECO draft v0.7 dated 2026-07-18

## Purpose

This contract makes certification-oriented sustainability evidence declarations first-class ShortHand syntax without allowing the compiler or generated software to grant an external certification. The declarations are compiled metadata/evidence inputs, not a certification decision.

The supplied C3-ECO materials are consultation and pilot drafts. The v0.6 standard is the canonical gate/domain source for this repository; the v0.7 eligibility and all-inclusive documents are an overlay for inclusive software classes, commercial routing and claim wording. Bronze is the first certification level. Registered and Measured Candidate are readiness states, not certified levels.

C3-ECO assessment is technology-neutral. A programming language, framework, cloud, backend or model is not inherently green. Evidence must compare required useful work within a declared product/version, workload, functional unit, system boundary, deployment/hardware context, relevant geography and validity period.

## Declaration kinds

The v1 language provides ten named blocks: `certification`, `functional_unit`, `workload`, `boundary`, `measurement_plan`, `ai_lifecycle`, `rag_pipeline`, `token_budget`, `model_routing` and `guardrails`. Each v1 block uses string-valued fields so provenance-rich values and units remain explicit and serializable. Beta-0.7 adds the separate `shorthand.c3eco.profile.v2` contract documented in `docs/c3eco_certification_profile.md`; a `certification_profile` links typed versions of the applicable v1 declarations without silently reinterpreting legacy strings.

```short
certification release_candidate {
  version "1.0";
  owner "platform-team";
  software_class "ai-application";
  deployment_mode "production";
  geography "IN";
  validity_period "30d";
};
```

## Required semantic fields

| Block | Required fields |
| --- | --- |
| `certification` | `version`, `owner`, `software_class`, `deployment_mode`, `geography`, `validity_period` |
| `functional_unit` | `denominator`, `success_condition`, `quality_threshold` |
| `workload` | `traffic_profile`, `batch_size`, `concurrency`, `warmup_runs`, `measured_runs`, `cache_state` |
| `boundary` | `include`; `exclude` additionally requires `evidence` naming component, reason and materiality |
| `measurement_plan` | `instrument`, `carbon_factor`, `sampling`, `uncertainty`, `retention`; `carbon_factor` must name source and unit |
| `ai_lifecycle` | `role`, `model_provider`, `lifecycle_scope`, `training_included`, `fine_tuning_included`, `evaluation_included` |
| `rag_pipeline` | `embedding_model`, `vector_db`, `retrieval_top_k`, `cache_policy` |
| `token_budget` | `input_tokens_p95`, `output_tokens_p95`, `cache_hit_rate_min_percent` |
| `model_routing` | `route`, `fallback` |
| `guardrails` | `functional_tests`, `accuracy`, `p95_latency_ms`, `error_rate_percent`, `security_scan`, `accessibility`, `privacy_telemetry` |

## Fail-closed diagnostics

- `SHD5101`: duplicate declaration.
- `SHD5102`: required field missing.
- `SHD5103`: invalid field or incomplete provenance/boundary evidence.
- `SHD5104`: unsafe self-certification claim.

Typed profile diagnostics `SHD5201` through `SHD5208` cover literal types, closed domains, ranges, references, validity, materiality and missing v2 fields.

Fields such as `official_certification_granted`, `certified_level`, `certificate_id` and `certified` are rejected. Candidate reports always retain `official_certification_granted:false`.

## Compiler/evidence behavior

The AST preserves declaration kind, name and fields. The semantic analyzer validates the contract. LLVM generation emits `shorthand.c3eco_declaration` metadata, so declarations do not introduce accidental runtime work. Candidate evidence serializes the same structure under `c3eco_language_contract: shorthand.c3eco.language.v1`.

Measured energy/carbon/cost scoring and authority-ready signed auditor lineage are deliberately outside this contract and remain subsequent roadmap work. Profile v2 does not change that claim boundary.

## Measurement and claims boundary

Candidate metrics may use units such as Wh per API request, kWh per 1,000 successful requests, J per inference, gCO2e per inference or kWh per GB processed. The claim chain is energy per functional unit, carbon per functional unit using a disclosed factor, and electricity cost using a disclosed tariff. PR82 declarations do not perform that calculation. PR89 owns measurement, carbon allocation, uncertainty and cost-workbook evidence.

Evidence quality must increase with the claimed certification level, from transparent MQ1 estimation/basic telemetry at Bronze toward independently verified lifecycle evidence at Diamond. A score cannot override a failed critical gate. No improvement counts when required functionality, accuracy, reliability, security, privacy, safety or accessibility is degraded. Offsets, renewable hosting and avoided-impact claims remain separate from the product-level footprint.

Draft v0.7 adds cost calculation where claimed as G6 and shifts its G7-G12 numbering relative to v0.6. `docs/c3eco_traceability.tsv` keeps the v0.6 G1-G14 keys, records each v0.7 alias and maps conditional cost calculation into canonical G5. This prevents the overlay from silently changing the security, safeguard, repeatability, retention, claim-integrity, offset or recertification controls.
