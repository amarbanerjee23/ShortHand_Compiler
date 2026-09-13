# Native CPU energy qualification

ai_qualification_contract: shorthand.ai.cpu_qualification.v1
current_github_pr: 95
controlled_hardware_evidence: pending
production_claim: false
comparative_energy_claim: false
official_certification_granted: false

GitHub PR95 extends AIRuntime, BackendRegistry-compatible backends and TorchTrainer with an opt-in CPU qualification tool. Ordinary ShortHand grammar, semantics, interpreter behavior, MLIR lowering and exported C ABI remain unchanged. The backend disables vendor telemetry before ORT environment initialization (`ORT_DISABLE_TELEMETRY=1`) and through `DisableTelemetryEvents`; local ShortHand evidence is retained. CPU ONNX execution is the existing qualified backend; accelerator inventory is not accelerator execution evidence.

## Execution and selection

`shorthand_ai_qualify` profiles explicit CPU thread candidates (1, 2, 4, 8 capped by available CPUs). The single-thread CPU baseline is mandatory. Each candidate owns one prepared ONNX Runtime session, with sequential execution, one inter-op thread, explicit intra-op threads, disabled spinning and basic graph optimizations. Warmup and repeated inference reuse that session. Input/output tensors must be finite FP32 with matching concrete shapes. Dynamic dimensions require concrete workload shapes. This version supports one FP32 input and one FP32 output; token-based language models, multiple outputs and accelerator execution remain outside scope.

The ONNX envelope inspector streams over tensor payloads. It rejects malformed envelopes, unsafe external paths and quantized tensor/operator representations. External weights require an exact filename, size and SHA-256 manifest. No external weight contents enter compiler source or IR. An operator must supply immutable model artifacts during execution; qualification is not a sandbox for hostile third-party model code. Enforce a process/container memory limit in controlled runs; the tool additionally bounds model and tensor sizes and rejects excessive Linux process peak RSS. Peak RSS is process-wide and cumulative, not a per-candidate allocation estimate. On other platforms that runtime peak measurement is unavailable, not zero-memory evidence.

Successful candidates must preserve numeric tolerances, quality threshold, FP32/no-quantization policy and any latency SLA. Only directly measured, instrument-qualified J/FU can select an energy winner. The minimum mean J/FU wins; equal energy is resolved by uncertainty, latency, CPU preference and candidate ID. Uncertainty is the supplied instrument percentage plus twice the observed sample standard deviation divided by mean; it is not a statistical confidence interval. A bounded comparative-energy flag additionally requires non-overlapping uncertainty intervals and supplied guardrail evidence. It is scoped to this workload and device; it is never a lowest-carbon-language or certification claim.

Without qualified telemetry, execution may use the single-thread baseline with `cpu_baseline_without_energy_optimization`. `require_measured_energy: true` instead returns failure and a report with no selected candidate. Synthetic planner tests always use `synthetic_test` and cannot produce measured production selection.

Latency covers execution and numerical validation, normalized by completed-batch functional units. Session preparation, warmup and all failed attempts are retained as separate phases. The outer qualification window includes candidate search and its bookkeeping overhead. Artifact acquisition/hash validation before profiling and final report export are excluded. It overlaps the inner measurements and must never be added to them. This PR makes no automatic amortization claim; deployment avoids repeating the candidate search.

## Real measurements

The Linux collector discovers the real `/sys/class/powercap` tree, confirms sysfs, bounds traversal and rejects unsafe aliases, malformed counters and inconsistent domains. It records raw `energy_uj` and `max_energy_range_uj`, timestamps and domain names. Only top-level `package-*` domains contribute to the CPU package total. Nested core/DRAM readings remain visible but are not added to their parent. Duplicate package aliases fail closed. A validated per-package maximum power bound is required to rule out ambiguous multiple counter wraps; it is never substituted for measured joules. Long windows exceeding the unambiguous-wrap bound fail closed. RAPL is package energy, not process energy. There is no silent idle subtraction. See the [Linux powercap interface](https://docs.kernel.org/power/powercap/powercap.html).

`physical_meter` reads a bounded CSV with exact header `unix_time_s,power_w`. It rejects non-finite/negative watts, non-increasing timestamps, oversized traces and unbracketed execution windows. Trapezoidal integration interpolates only the two window endpoints and retains the raw samples. An independent logger must flush samples with the same synchronized Unix clock; the collector waits at most one second for a bracketing sample. Acquisition waits are excluded from that inner window and included in enclosing qualification/training windows. No network or privileged meter driver is installed. Unsupported platforms or unavailable meters produce explicit unavailable evidence.

Instrument records require identity, calibration ID/date, validation reference, explicit uncertainty, measurement boundary and isolation description for claim eligibility. These are operator-supplied provenance, not an independent calibration certificate. Concurrent system work, meter sampling interval and system boundary must be assessed during controlled qualification. GPU/NVML energy is not implemented or required in this CPU-only increment.

## Training

`TorchTrainer::trainQualification` runs a native reference CNN: two 3x3 convolution/ReLU layers, global average pooling and a two-class FP32 classifier, with cross-entropy backward propagation and ordered SGD reduction. It has 354 trainable parameters. Defaults are seed 42, learning rate 0.1, batch 8, 64 training samples, 32 independent validation samples and 16 epochs. Inputs are synthetic noisy horizontal/vertical stripes; this is a nontrivial synthetic correctness workload, not an ImageNet model or a general autodiff/LibTorch qualification claim. The legacy trainer and `short_ai_train` behavior remain unchanged.

Per-sample gradients use bounded native workers and deterministic ordered reduction. Equivalent threads use the same initialization, data, optimizer, batch, learning rate and validation threshold. Tests independently check finite-difference gradients, loss reduction, target accuracy and bit-identical parameters across thread counts. Strict TSan executes actual parallel work. Reports retain energy per optimizer step, epoch and complete training run, including failures. Whole-run J/sample includes data generation, initialization, training and validation; checkpoint I/O is excluded. These nested readings are alternative boundaries, not additive components.

## Commands and profile integrity

Build `shorthand_ai_qualify` using the existing CMake ONNX CPU SDK options. SDK-off builds still execute native training and collector/planner tests; attempted ONNX execution fails explicitly. Example native training configuration:

```json
{"schema":"shorthand.ai.cpu_qualification.config.v1","mode":"training","workload":"reference_cnn","threads":[1,2],"warmups":1,"trials":3,"energy_source":"unavailable"}
```

```sh
shorthand_ai_qualify qualify config.json report.json
sha256sum report.json
shorthand_ai_qualify execute config.json report.json TRUSTED_REPORT_SHA256 execution.json
```

The trusted digest must come from the operator's approved profiling pipeline. A checksum is integrity binding, not a signature or an independent approval. Deployment requires the unchanged configuration, primary and external model hashes, CPU hardware fingerprint, compiler revision, backend runtime version, precision, batching and allowed thread count. Stale, modified or unselected profiles fail before execution. The `execute` CLI repeats the configured deterministic qualification workload through a single prepared session; applications can use the additive `AIRuntime::prepare` API with their own validated tensors. No automatic production task-accuracy inference is made from synthetic inputs.

Configuration parsing reuses the bounded C3-ECO JSON parser. Unknown fields, duplicates, invalid types and invalid protocol limits fail. `scripts/collect_energy.sh` preserves its existing no-argument probe and dispatches arguments to the native tool. No Python interpreter is required by the native tool or deployed runtime.

## Existing C3-ECO evidence chain

```sh
shorthand_ai_qualify export-workbook report.json TRUSTED_REPORT_SHA256 accounting.json raw.tsv
shorthand_c3eco_measure raw.tsv workbook.csv workbook.json
```

Export uses the unchanged `shorthand.c3eco.measurement_workbook.v1` input contract and exports the outer measured qualification window once. Unavailable and synthetic measurements cannot export. The operator supplies `record_id`, `component`, `allocation_fraction`, `pue`, `carbon_factor_gco2e_per_kwh`, `factor_source`, `factor_date`, `tariff_per_kwh`, `tariff_currency`, `tariff_source`, `measurement_quality`, `data_quality` and `evidence_ref`. There are no invented default carbon factors or tariffs. The existing native workbook validates units, dates, allocation/PUE, provenance and double counting; accounting invalidity fails there. Existing assessment, eco-regression and signed auditor tooling remain the authorities for candidate evidence and claims. Export is not official certification.

## Two workload tiers

Normal CI runs deterministic native collector/selection/gradient tests, actual FP32 ONNX identity and external MatMul inference, malformed/hash/precision/profile failures and workbook serialization/accounting tests. Artificial meter values occur only in temporary unit/serialization fixtures, never retained as hardware evidence. Both mandatory MLIR compiler lanes also run the live ONNX CPU qualification gate, including strict ASan/LSan/UBSan. Make and CTest include the new suite; TSan covers the new training workers; CodeQL builds the native tool.

The controlled registry pins ONNX model-zoo revision `4f43949841cb55a0b98dc8fcd045431ccafd9f96`, exact hashes, sizes and upstream license references for MobileNetV2-12, Tiny YOLOv2-8 and ViT tiny patch16/224. `tests/ai_energy/controlled/models.json` records the distinctions: pretrained MobileNet, standard small YOLO, and an upstream timm vision-transformer export whose pretrained lineage is not asserted. ViT is not a GPT/LLM. Synthetic inputs establish numerical/top-1/tensor agreement only, not ImageNet accuracy or detection mAP. Weight artifacts are cached externally, not embedded in the repository.

```sh
python3 scripts/qualify_controlled_cpu_models.py --tool /path/to/shorthand_ai_qualify --cache /model/cache --output /evidence --download --instrument instrument.json --guardrail-ref audited-guardrails
```

The controlled command fails if required models, hashes, calibration or measured energy are unavailable. `--execution-only` explicitly selects a numerical execution run with energy unavailable; it cannot satisfy the controlled energy gate. Python is used only for acquisition/test orchestration. Model and license provenance is recorded next to results. The 120M FP32 external-weight fixture is generated separately:

```sh
python3 scripts/create_ai_weight_scalability_fixture.py /fixture/output
shorthand_ai_qualify qualify /fixture/output/qualification.json /fixture/output/report.json
```

It streams 480,000,000 bytes of deterministic weights into a sidecar file, with a small ONNX MatMul graph and a hash manifest. This tests weight-file scaling; it is explicitly synthetic, not a trained language model, and does not by itself establish energy efficiency.

## Release gates still open

This increment does not close roadmap PR94 representative application/data-quality evidence, PR95 equivalent Python workload performance/energy comparisons, or PR97 independent calibrated measurement qualification. CPU-only deployment remains the production backend boundary. Standard dataset accuracy, real token-based language-model execution, GPU execution, broad training stacks, enterprise pilots, full lifecycle/cloud carbon accounting, protected signed release operation, independent reproduction and certification remain later gates. The conservative plan is ten increments including GitHub PR95 and nine afterward. G8 and the existing partial scoring domains remain partial; the historical PR93 assessment is unchanged.

Development validation on 2026-09-13 executed all three pinned standard model families with all four CPU thread candidates and no numerical/quality violations. The 120M FP32 fixture also executed with an approximately 988 MB process peak. These runs used explicit `energy_source: unavailable`; they are execution/scalability evidence only. Native tests report 56 assertions and 42 CLI cases. Hosted exact-head CI and calibrated physical measurements are separate release requirements.
