# Green AI evidence workflow for ShortHand

ShortHand does **not** grant a certification by itself. The Green AI tooling added in this repository produces transparent, machine-readable evidence that can support a C3-ECO-style Green Software / Green AI review.

## Green manifest DSL

A `.greenai` manifest is a sidecar DSL file for a `.short` application or native AI workflow. Green features are optional for backward compatibility, but manifests can opt into strict validation:

```text
green {
  product: "Green image classification inference"
  green_mode: "strict"
  certification_profile: "C3-ECO-AI"
  functional_unit: "1000_inferences"
  boundary: { include: ["inference", "data_loading", "model_serving"] exclude: [] embodied_hardware: "allocated" }
  measurement_quality: "MQ2"
  data_quality: "DQ2"
}
```

`green_mode` behavior:

- `off`: ignore green checks.
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

Strict mode requires `green.functional_unit`.

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

## Carbon calculation

The report generator uses:

```text
operational_carbon_gco2e = energy_kwh * grid_factor_kgco2e_per_kwh * 1000
carbon_per_functional_unit_gco2e = total_operational_carbon_gco2e / functional_unit_count
```

Training, inference, idle, network, and storage energy are tracked separately and then added into total operational energy. Accelerator energy should not be double-counted when already included in measured operational energy.

## Commands

Validate a manifest:

```bash
./tools/green_ai_tool.py validate examples/green_ai/image_classification.greenai --strict strict
```

Generate an audit evidence report:

```bash
./scripts/green-report examples/green_ai/image_classification.greenai --output green-report.json --strict strict
```

Run an eco-regression check against a baseline report:

```bash
./scripts/green-check examples/green_ai/image_classification.greenai --baseline green-baseline.json --threshold-percent 10
```

The eco-check fails when supported metrics regress by more than the configured threshold, including energy per functional unit, carbon per functional unit, memory peak, network per task, tokens per task, p95 latency-energy, and model size.

## Examples

- `examples/green_ai/image_classification.greenai`: image inference, int8 model metadata, target hardware, inference budgets.
- `examples/green_ai/llm_rag.greenai`: GenAI/RAG token budget, retrieval limit, model routing/cascade metadata.
- `examples/green_ai/training_pipeline.greenai`: training energy, data movement/storage controls, checkpoint policy.

## Limitations and assumptions

- The tool creates evidence for certification readiness; it does not certify the software.
- If real power telemetry is unavailable, values should be declared as estimates and classified as `MQ1`/`DQ1` where appropriate.
- The manifest parser is dependency-free and intentionally conservative; complex expressions should be precomputed by measurement systems before report generation.
- Existing `.short` programs remain valid. Green manifests are opt-in unless a workflow explicitly runs strict validation.
