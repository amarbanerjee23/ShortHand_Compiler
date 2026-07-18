# C3-ECO Certification Language Upgrade Plan

This plan maps C3-ECO v0.6 certification expectations to concrete ShortHand language and compiler upgrades. The goal is to make ShortHand commercially useful for companies building AI software that can generate audit-ready C3-ECO evidence reports.

## Current position

ShortHand currently has a controlled beta / pilot language foundation. It has AI and GreenAI syntax, AST metadata, semantic validation, fallback-aware runtime behavior, evidence reporting, CI validation, and release-readiness tracking.

It is not yet sufficient for companies to build enterprise AI software that can directly pass C3-ECO certification. The missing work is mainly in compiled AI runtime lowering, real backend execution, complete evidence generation, measurement quality/data quality capture, carbon workbook generation, and claims control.

## Certification target

ShortHand should evolve into a certification-aware AI software language that can help an applicant produce:

1. product and version declaration,
2. software class declaration,
3. functional unit declaration,
4. boundary declaration,
5. workload and quality guardrail declaration,
6. energy and resource measurement records,
7. AI model and inference lifecycle records,
8. carbon and cost calculations,
9. MQ/DQ classification,
10. C3-ECO score inputs,
11. evidence pack index,
12. claim-safe public report text.

## Required ShortHand language upgrades

### C3L-1. Certification declaration block

Add a first-class declaration for product identity and certification scope.

Proposed syntax:

```short
certification product_name {
  version "1.2.0";
  owner "ExampleCo";
  release_date "2026-07-18";
  software_class S6_AI_GENAI;
  deployment_mode "kubernetes";
  geography "UK";
  cloud_region "uk-south";
  validity_period "12 months";
}
```

Why needed:

- C3-ECO gates require system identity, product version, owner, release date, deployment mode, software class, and scope.
- Without this, generated reports cannot reliably bind evidence to a certified product/version.

Implementation tasks:

- Lexer tokens for `certification`, `software_class`, `deployment_mode`, `geography`, `cloud_region`, and `validity_period`.
- Parser rule for certification block.
- AST node `AST_CERTIFICATION_DECLARATION`.
- Semantic validation for required fields.
- Evidence emitter fields for product identity.
- Negative tests for missing version, class, or owner.

### C3L-2. Functional unit and workload profile as language objects

The current `greenai_contract` stores functional unit text, but certification needs structured workload details.

Proposed syntax:

```short
functional_unit ai_inference_unit {
  denominator "1000 inferences";
  success_condition "classification completed";
  quality_threshold accuracy >= 90;
  latency_slo_ms p95 <= 500;
  error_rate_slo_percent <= 1;
}

workload image_classification_workload {
  traffic_profile "production_representative";
  batch_size 1;
  concurrency 32;
  warmup_runs 5;
  measured_runs 30;
  cache_state "declared";
}
```

Why needed:

- Functional unit, success criterion, quality constraints, workload, concurrency, sampling, and repeatability are mandatory for certification.
- It prevents artificial denominator inflation and toy workloads.

Implementation tasks:

- Add syntax for `functional_unit` and `workload`.
- Link workload to contract and measurement.
- Validate presence of denominator, success condition, quality guardrail, and sampling details.
- Add tests for invalid missing workload/functional-unit fields.

### C3L-3. Boundary declaration as a typed structure

Current boundary list is useful but not detailed enough for audit.

Proposed syntax:

```short
boundary ai_app_boundary {
  include compute, accelerator, storage, network, ci_cd, thirdparty_ai_api;
  exclude client_device reason "server-side API only" materiality_percent 0.5;
  thirdparty_service "LLMProviderX" role model_provider boundary conservative_estimate;
}
```

Why needed:

- C3-ECO requires included/excluded layers, materiality, third-party boundaries, and conservative treatment for opaque model-provider boundaries.

Implementation tasks:

- Add typed boundary entries with include/exclude/reason/materiality.
- Validate cumulative omissions and flag material exclusions.
- Emit boundary checklist in evidence report.
- Add claim cap logic when model-provider boundary is excluded.

### C3L-4. Measurement plan block

Certification requires a measurement plan before formal testing.

Proposed syntax:

```short
measurement_plan certification_run {
  instrument rapl source "intel_rapl" mq MQ2;
  instrument gpu source "nvml" mq MQ2;
  carbon_factor location value 0.17109 unit "kgCO2e/kWh" source "UK 2026 GHG factors";
  electricity_price value 0.2414 currency "GBP" unit "per_kWh" source "DESNZ Q1 2026";
  sampling runs 30 duration_seconds 600 interval_seconds 1;
  uncertainty target_percent 12;
  retention "12 months";
}
```

Why needed:

- C3-ECO requires instruments, sampling design, uncertainty, carbon factors, MQ/DQ classes, and evidence retention.
- UK savings examples require explicit electricity price and carbon factor sources.

Implementation tasks:

- Add measurement plan AST.
- Validate MQ/DQ classes and factor sources.
- Emit measurement plan in JSON report.
- Add strict-mode error when carbon or price savings are claimed without factors.

### C3L-5. Resource and telemetry capture primitives

Add language/runtime support for measuring resource use per functional unit.

Proposed syntax:

```short
measure energy for ai_inference_unit using certification_run {
  cpu_energy_kwh from rapl;
  gpu_energy_kwh from nvml;
  network_gb from telemetry;
  storage_gb_month from cloud_meter;
  tokens from model_server;
  retries from apm;
}
```

Why needed:

- Certification needs energy, CPU, GPU, storage, network, tokens, retries, throughput, latency, and quality data.
- AI systems need per-inference or per-1,000-token metrics.

Implementation tasks:

- Add runtime measurement adapters.
- Support RAPL/NVML availability checks.
- Store unavailable tools honestly rather than fabricating data.
- Add tests that unavailable telemetry yields `measurement_status: unavailable` or `declared_budget_only`.

### C3L-6. AI lifecycle declaration

AI certification must separate model provider, application deployer, integrator, and auditor responsibilities.

Proposed syntax:

```short
ai_lifecycle classifier_lifecycle {
  role application_deployer;
  model_provider "third_party" boundary excluded_conservative;
  lifecycle_scope inference, rag, routing, caching, monitoring;
  training_included false;
  fine_tuning_included false;
  evaluation_included true;
}
```

Why needed:

- Third-party AI apps can certify orchestration, prompts, RAG, routing, caching, usage accounting, and boundaries, but must disclose excluded provider training boundary.
- Own model training/fine-tuning requires stronger evidence and usually Gold or higher.

Implementation tasks:

- Add AI lifecycle AST and semantic validation.
- Require training/fine-tuning evidence when model-level green claims are made.
- Cap level if own training is excluded from a model-lifecycle claim.
- Emit role separation in evidence pack.

### C3L-7. RAG, token, cache, and routing metrics

Add first-class concepts for GenAI workload efficiency.

Proposed syntax:

```short
rag_pipeline support_answering {
  embedding_model "embed-small";
  vector_db "pgvector";
  retrieval_top_k 5;
  cache_policy semantic_cache ttl_seconds 86400;
}

token_budget support_answering {
  input_tokens_p95 4000;
  output_tokens_p95 800;
  cache_hit_rate_min_percent 30;
}

model_routing support_router {
  route low_complexity to small_model;
  route high_complexity to large_model;
  fallback large_model;
}
```

Why needed:

- C3-ECO AI module requires token energy, RAG traces, routing config, caching, prompt classes, quality-energy frontier, and usage accounting.

Implementation tasks:

- Add syntax and AST for RAG pipeline, token budget, and model routing.
- Validate referenced models exist.
- Emit token, cache, route, and fallback metrics in evidence JSON.

### C3L-8. Carbon and cost calculation built-ins

Add built-ins that compute energy, carbon, and bill savings from measured kWh.

Proposed syntax:

```short
carbon_workbook c3eco_report {
  component compute activity_kwh measured_cpu + measured_gpu factor location;
  component network activity_gb network_gb factor conservative_proxy;
  component thirdparty activity_requests model_api_calls factor provider_or_conservative;
  calculate location_based;
  calculate market_based optional;
  cost_savings electricity_price;
}
```

Why needed:

- C3-ECO requires a workbook with component, boundary status, functional unit mapping, activity data, emission factor, allocation factor, DQ/MQ, uncertainty, conservative CO2e, and evidence reference.
- UK business-case reports need cost savings from kWh reduction and disclosed price, not unsupported marketing assumptions.

Implementation tasks:

- Add carbon workbook data model.
- Compute `E_call_kWh`, `Carbon_call_kgCO2e`, `Cost_call_GBP`, annual kWh saved, annual CO2e avoided, and bill savings.
- Keep offsets and avoided impact separate.
- Emit CSV/JSON workbook artifacts.

### C3L-9. Certification scoring and level estimator

Add a conservative local estimator, not an official certificate issuer.

Proposed syntax:

```short
scorecard c3eco_score {
  domain A measurement_integrity score 4 evidence "measurement_plan.json";
  domain B runtime_efficiency score 3 evidence "runtime_metrics.json";
  domain G ai_efficiency score 4 evidence "ai_lifecycle.json";
  minimum_level_target Gold;
}
```

Why needed:

- Certification levels depend on mandatory gates, score, MQ/DQ, caps, uncertainty, and minimum domain thresholds.
- The language should help teams prepare for audit but must not self-grant certification.

Implementation tasks:

- Add scorecard AST and validator.
- Generate candidate score and blockers.
- Mark output as `candidate_assessment_only` unless external certifier signs.
- Add anti-greenwashing claim control.

### C3L-10. Claim-safe report generation

Add a report mode that emits certificate-ready but claim-safe text.

Proposed CLI:

```bash
short_hand app.short c3eco-report --output c3eco_report.json --format json
short_hand app.short c3eco-report --output c3eco_public_report.md --format markdown
short_hand app.short c3eco-workbook --output c3eco_workbook.csv
```

Report must include:

- product/version,
- certified or candidate level,
- functional unit,
- boundary,
- MQ/DQ classes,
- energy/unit,
- gCO2e/unit,
- uncertainty,
- location/market-based split,
- score summary,
- limitations/exclusions,
- required improvements,
- permitted claim text,
- surveillance triggers.

Implementation tasks:

- Add CLI modes `c3eco-report`, `c3eco-workbook`, and `c3eco-check`.
- Add JSON schema.
- Add Markdown report template.
- Add claim policy scanner.
- Add tests for report fields and restricted claims.

### C3L-11. Quality, security, privacy, and accessibility guardrails

Add guardrail syntax that prevents energy improvements from passing if quality or safety falls.

Proposed syntax:

```short
guardrails certified_mode {
  functional_tests pass;
  accuracy >= 90;
  p95_latency_ms <= 500;
  error_rate_percent <= 1;
  security_scan critical_vulns == 0;
  accessibility wcag_level "AA";
  privacy_telemetry declared;
}
```

Why needed:

- C3-ECO denies certification if efficiency is achieved by weakening security, safety, privacy, accessibility, reliability, correctness, or quality.

Implementation tasks:

- Add guardrail AST.
- Link guardrails to workload/certification mode.
- Fail `c3eco-check` if guardrails are absent or failed.
- Store guardrail evidence references.

### C3L-12. CI/CD and eco-regression gates

Add release-to-release eco-regression testing.

Proposed CLI:

```bash
short_hand app.short c3eco-check --baseline baseline_report.json --threshold-percent 10
```

Why needed:

- C3-ECO surveillance requires corrective action when material eco-regression occurs.
- CI should prevent regressions while keeping tests intact.

Implementation tasks:

- Add baseline comparison mode.
- Fail CI on >10% material regression unless explicitly approved.
- Store report artifacts.
- Keep existing setup, validation, smoke, Makefile, sanitizer, CMake, and CTest gates unchanged.

## Implementation phases

### Phase 1: Certification-aware language model

Deliver:

- certification block,
- structured functional unit,
- boundary block,
- measurement plan,
- guardrails,
- semantic validation,
- JSON report skeleton.

### Phase 2: AI/GenAI evidence support

Deliver:

- AI lifecycle block,
- RAG/token/cache/routing declarations,
- backend matrix,
- runtime/evidence metadata,
- model/provider boundary disclosure.

### Phase 3: Measurement and workbook generation

Deliver:

- telemetry adapters,
- carbon workbook,
- cost savings calculations,
- MQ/DQ classification,
- uncertainty handling,
- evidence references.

### Phase 4: Enterprise report and claims control

Deliver:

- `c3eco-report`,
- public Markdown report,
- permitted claim text,
- restricted claim scanner,
- evidence pack index,
- surveillance trigger list.

### Phase 5: Commercial hardening

Deliver:

- complete grammar/spec,
- full conformance suite,
- source-aware diagnostics,
- module/import/package model,
- formatter/linter/editor support,
- SBOM/signing/security scans,
- real ONNX Runtime execution,
- deployment validation.

## CI/CD rule

All implementation PRs must preserve and pass:

- setup build infrastructure,
- strict language validation,
- smoke tests,
- feature plan status check,
- Makefile test suite,
- sanitizer tests,
- CMake configure/build,
- CTest,
- artifact upload.

New certification features must add positive and negative tests. No PR should remove or weaken existing validation to pass CI.

## Commercial claim position

ShortHand may be marketed as a certification-aware AI software language only after Phase 1 and Phase 2 have working code and reports. It may be marketed as a full enterprise production language only after Phases 1-5 are complete, CI passes, and evidence generation is demonstrated on at least one realistic AI application.
