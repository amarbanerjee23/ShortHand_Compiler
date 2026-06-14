# Green AI evidence workflow for ShortHand

ShortHand does **not** grant a certification by itself. The Green AI tooling in this repository produces transparent, machine-readable evidence that can support a C3-ECO-style Green Software / Green AI review.

The official Green AI workflow is compiled C++ only. Python is not required for validation, report generation, carbon estimation, or eco-regression checks.

## `.short` executable programs vs `.greenai` evidence manifests

ShortHand uses two complementary layers:

1. `.short` programs express executable logic and compiled AI/GreenAI primitives such as `greenai(...)` and `ai_infer(...)`.
2. `.greenai` manifests express audit metadata that should not be hard-coded into executable logic, such as functional units, system boundaries, carbon factors, MQ/DQ classes, energy budgets, evidence references, and measurement assumptions.

Both layers are handled by compiled C++ tooling.

## Green manifest DSL

A `.greenai` manifest is a sidecar DSL file for a `.short` application or native AI workflow. Green features are optional for backward compatibility, but manifests can opt into strict validation:

```text
green {
  product: "Green image classification inference"
  green_mode: "strict"
  certification_profile: "C3-ECO-AI"
  functional_unit: "1000_inferences"
  success_criteria: { accuracy_min: 0.92 p95_latency_ms_max: 250 error_rate_max: 0.01 }
  boundary: { include: ["inference", "data_loading", "model_serving"] exclude: [] embodied_hardware: "allocated" }
  measurement_quality: "MQ2"
  data_quality: "DQ2"
}
```

`green_mode` behavior:

- `off`: parse but ignore green checks.
- `advisory`: report warnings without failing validation.
- `strict`: missing mandatory evidence fields are validation errors.

## Functional units

A functional unit is the useful-work denominator for energy and carbon intensity. Supported examples include:

- `1000_inferences`
- `1000_tokens`
- `1_training_run`
- `1_GB_processed`
- `1_user_hour`
- `1000_api_requests`
- `1_batch_job`

Strict mode requires `green.functional_unit` and `green.success_criteria`.

## Boundaries and carbon factors

The `boundary` block declares included and excluded system components. The `carbon` block declares accounting method, region, and grid factor:

```text
carbon {
  accounting_method: "location_based"
  region: "IN-WEST"
  grid_factor_kgco2e_per_kwh: 0.475
  report_market_based_separately: true
  offsets_allowed_in_core_score: false
}
```

Offsets must not be subtracted from the core software carbon footprint. Market-based and location-based accounting values must remain separable.

## Measurement quality and data quality

Measurement quality classes:

- `MQ0`: unsupported claim; no credible measurement.
- `MQ1`: transparent estimate using documented assumptions.
- `MQ2`: telemetry-based measurement.
- `MQ3`: instrumented or calibrated measurement with repeated runs and idle baseline.
- `MQ4`: independently verified measurement.

Data quality classes:

- `DQ0`: unknown or unsupported input data.
- `DQ1`: generic estimate.
- `DQ2`: provider/runtime telemetry.
- `DQ3`: system-specific measured data.
- `DQ4`: independently verified or calibrated data.

In strict mode, `MQ0` and `DQ0` are not certification-ready.

## Carbon calculation

The C++ report generator uses:

```text
operational_carbon_gco2e = energy_kwh * grid_factor_kgco2e_per_kwh * 1000
total_carbon_gco2e = operational_carbon_gco2e + separately_declared_additive_carbon
carbon_per_functional_unit_gco2e = total_carbon_gco2e / functional_unit_count
```

Training, fine-tuning, evaluation, inference, idle, network, storage, client, CI/CD, update, and third-party AI API energy can be tracked separately and added into total operational energy. Accelerator energy should not be double-counted when already included in measured operational energy.

## Commands

Build the compiled C++ Green AI tool:

```bash
make -C Compiler_new_ws/Short_Hand/src green_ai_tool
```

Validate a manifest:

```bash
./Compiler_new_ws/Short_Hand/build/green_ai_tool validate examples/green_ai/image_classification.greenai --strict strict
```

Generate an audit evidence report:

```bash
./Compiler_new_ws/Short_Hand/build/green_ai_tool report examples/green_ai/image_classification.greenai --output green-report.json --strict strict
```

Run an eco-regression check against a baseline report:

```bash
./Compiler_new_ws/Short_Hand/build/green_ai_tool check examples/green_ai/image_classification.greenai --baseline green-baseline.json --threshold-percent 10
```

Wrapper scripts call the compiled C++ binary and build it if missing:

```bash
./scripts/green-report examples/green_ai/image_classification.greenai --output green-report.json --strict strict
./scripts/green-check examples/green_ai/image_classification.greenai --baseline green-baseline.json --threshold-percent 10
```

The eco-check fails when supported metrics regress by more than the configured threshold, including energy per functional unit, carbon per functional unit, training energy, inference energy, memory peak, network per task, tokens per task, p95 latency-energy, and model size.

## Examples

- `examples/green_ai/image_classification.greenai`: image inference, int8 model metadata, target hardware, inference budgets.
- `examples/green_ai/llm_rag.greenai`: GenAI/RAG token budget, retrieval limit, model routing/cascade metadata.
- `examples/green_ai/training_pipeline.greenai`: training energy, data movement/storage controls, checkpoint policy.

## Certification-readiness mapping

| ShortHand evidence feature | Certification-readiness concept |
|---|---|
| `green.functional_unit` | useful-work denominator |
| `green.success_criteria` | quality and anti-gaming guardrail |
| `green.boundary` | system boundary |
| `carbon` | carbon accounting factor and reporting basis |
| `budgets` | eligibility benchmark / guardrail |
| `measurements` | evidence inputs |
| `model.green_model` | model proportionality and compression |
| `train model ... green_train` | training lifecycle energy |
| `infer endpoint ... green_infer` | inference energy |
| `llm task ... green_llm` | token efficiency |
| `rag pipeline ... green_rag` | retrieval/embedding efficiency |
| `model_route ... green_route` | small-to-large model routing |
| `target ... green_target` | hardware/runtime target |
| `data pipeline ... green_data` | data movement/storage efficiency |
| `report` command | audit evidence pack |
| `check` command | eco-regression control |

## Limitations and assumptions

- The tool creates evidence for certification readiness; it does not certify the software.
- If real power telemetry is unavailable, values should be declared as estimates and classified as `MQ1`/`DQ1` where appropriate.
- The manifest parser is dependency-free C++ and intentionally conservative; complex expressions should be precomputed by measurement systems before report generation.
- Existing `.short` programs remain valid. Green manifests are opt-in unless a workflow explicitly runs strict validation.
- Missing telemetry is never fabricated as real measurement.
