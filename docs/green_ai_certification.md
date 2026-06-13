# ShortHand Green AI certification-readiness workflow

ShortHand produces Green AI **evidence** for certification readiness. It does not grant certification. Every generated report includes the note: “Evidence report only; this tool does not grant certification.”

## C++/LLVM-first direction

The official Green AI path is now compiled C++ only. The former Python manifest utility has been moved to `deprecated/python_tools/` for historical reference and is not used by official commands, tests, examples, or documentation.

The supported binaries are:

```bash
make -C Compiler_new_ws/Short_Hand/src green_ai_tool
./Compiler_new_ws/Short_Hand/build/green_ai_tool validate app.greenai --strict strict
./Compiler_new_ws/Short_Hand/build/green_ai_tool report app.greenai --output green-report.json --strict strict
./Compiler_new_ws/Short_Hand/build/green_ai_tool check app.greenai --baseline green-baseline.json --threshold-percent 10
```

## `.short` versus `.greenai`

Use `.short` for executable compiled behavior: control flow, native AI inference requests through `ai_infer(...)`, and runtime energy summaries through `greenai(...)`. Use `.greenai` sidecar manifests for audit metadata: functional units, boundaries, carbon factors, model necessity, measurement/data quality, budgets, assumptions, and evidence files.

## In-language primitives

```short
ai_infer("models/demo.onnx", "1,3", "0.1,0.2,0.3");
greenai("edge_model_v1", inferences, watts, seconds);
```

`greenai(...)` computes joules as `watts * seconds` and reports inferences per joule. `ai_infer(...)` is implemented in C++ and routes to ONNX Runtime when the optional SDK is enabled; otherwise it reports a clear native fallback message. No Python runtime is involved.

## Manifest blocks

The C++ parser supports identifiers, strings, numbers, booleans, arrays, nested maps, comments, named blocks, and multi-word block headers such as `infer endpoint classify` and `data pipeline training_data`.

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
  audit_evidence: ["onnxruntime_profile.json", "power_meter_run_42.csv"]
}

carbon {
  accounting_method: "location_based"
  region: "IN-WEST"
  grid_factor_kgco2e_per_kwh: 0.475
  offsets_allowed_in_core_score: false
}

budgets {
  energy_per_inference_j_max: 200
  carbon_per_1000_inferences_gco2e_max: 50
  memory_peak_mb_max: 2048
}
```

## Green modes

- `off`: parse manifests but skip Green AI validation.
- `advisory`: warnings are emitted, but validation remains usable.
- `strict`: mandatory evidence gaps fail validation.

Strict mode requires functional unit, success criteria, boundary, MQ/DQ classes, carbon accounting method, region, grid factor, offset policy, budgets, and deployment targets when models/deployments are declared.

## Functional units and boundaries

Functional units define useful work denominators such as `1000_inferences`, `1000_tokens`, `1_training_run`, `1_GB_processed`, `1_user_hour`, `1000_api_requests`, and `1_batch_job`. Boundaries declare included/excluded lifecycle components such as training, inference, data loading, model serving, storage, network, monitoring, preprocessing, and checkpointing.

## Carbon calculation

The C++ tool calculates:

```text
operational_carbon_gco2e = energy_kwh * grid_factor_kgco2e_per_kwh * 1000.0
carbon_per_functional_unit_gco2e = total_carbon_gco2e / functional_unit_count
```

Operational energy components are summed from declared training, fine-tuning, evaluation, inference, idle, network, storage, client, CI/CD, update, and third-party API energy. Separately declared embodied, network, storage, and third-party API carbon can be added as component carbon. Offsets are reported separately and never subtracted from the core footprint. Accelerator energy must not be double-counted if it is already included in operational energy.

## MQ/DQ classes

Measurement quality:

- `MQ0`: unsupported claim
- `MQ1`: transparent estimate
- `MQ2`: telemetry-based measurement
- `MQ3`: instrumented or calibrated measurement with repeated runs and idle baseline
- `MQ4`: independently verified measurement

Data quality:

- `DQ0`: unknown or unsupported input data
- `DQ1`: generic estimate
- `DQ2`: provider/runtime telemetry
- `DQ3`: system-specific measured data
- `DQ4`: independently verified or calibrated data

Invalid classes fail. MQ0/DQ0 fail strict mode.

## Budgets and eco-regression

Budgets are emitted as structured results with `budget`, `limit`, `actual`, `unit`, and `status`. Supported statuses are `pass`, `fail`, `not_evaluated`, and `unknown`.

Eco-regression checks compare current evidence to a baseline for energy/carbon per functional unit, training/inference energy, memory/GPU memory, network, tokens, p95 latency, p95 latency-energy, model size, storage, and cross-region transfer. Regressions include remediation hints.

## Certification-readiness mapping

| ShortHand / `.greenai` feature | Certification-readiness concept |
| --- | --- |
| `green.functional_unit` | useful-work denominator |
| `green.boundary` | system boundary |
| `carbon` | carbon accounting and grid-factor declaration |
| `budgets` | benchmark guardrails |
| `measurements` | energy/resource evidence inputs |
| `model.green_model` | model proportionality and compression |
| `train_model.green_train` | training lifecycle energy |
| `infer_endpoint.green_infer` | inference energy and latency-energy |
| `llm_task.green_llm` | token/prompt efficiency |
| `rag_pipeline.green_rag` | retrieval and embedding-cache efficiency |
| `model_route.green_route` | small-to-large routing |
| `target.green_target` | hardware/runtime target |
| `data_pipeline.green_data` | data movement and storage |
| `green_ai_tool report` | audit evidence export |
| `green_ai_tool check` | eco-regression testing |

## Native AI libraries

ShortHand keeps ONNX Runtime C++ as the primary inference direction and LibTorch C++ as the primary training direction. Optional future backends include OpenVINO, TensorRT, llama.cpp/GGML/GGUF, FAISS, OpenCV, Arrow/Parquet, oneDNN/DNNL, XNNPACK, Eigen, SentencePiece, and C++ tokenizer integrations. These SDKs are optional and must not be required to validate Green AI manifests.

## Limitations

- Real energy telemetry is not fabricated. User-supplied estimated watts should be classified as MQ1 unless independent telemetry is available.
- The C++ manifest parser is intentionally dependency-free and conservative.
- Native runtime power telemetry hooks such as RAPL/NVML are future enhancements.
- Sidecar evidence is still useful for audit metadata even as `.short` remains the executable language.
