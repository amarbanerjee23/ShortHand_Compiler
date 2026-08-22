# ShortHand C3-ECO first-class language contract

c3eco_language_contract_version: shorthand.c3eco.language.v1
production_claim: false
official_certification_granted: false

## Purpose

This contract makes certification-oriented sustainability evidence declarations first-class ShortHand syntax without allowing the compiler or generated software to grant an external certification. The declarations are compiled metadata/evidence inputs, not a certification decision.

## Declaration kinds

The language provides ten named blocks: `certification`, `functional_unit`, `workload`, `boundary`, `measurement_plan`, `ai_lifecycle`, `rag_pipeline`, `token_budget`, `model_routing` and `guardrails`. Each block uses string-valued fields so provenance-rich values and units remain explicit and serializable.

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

Fields such as `official_certification_granted`, `certified_level`, `certificate_id` and `certified` are rejected. Candidate reports always retain `official_certification_granted:false`.

## Compiler/evidence behavior

The AST preserves declaration kind, name and fields. The semantic analyzer validates the contract. LLVM generation emits `shorthand.c3eco_declaration` metadata, so declarations do not introduce accidental runtime work. Candidate evidence serializes the same structure under `c3eco_language_contract: shorthand.c3eco.language.v1`.

Measured energy/carbon/cost scoring and authority-ready signed auditor lineage are deliberately outside this contract and remain subsequent roadmap work.
