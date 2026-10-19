# PR123 retained runtime observations

This snapshot records both completed CI evidence runs from 2026-10-18. It supports review of the borrowed-input and deferred-observation change. It is descriptive shared-runner evidence, not a confidence-bound speedup, energy-saving or language-wide claim. Later documentation-only commits do not change the measured revision identified below.

- Measured runtime revision: `ca62eca1dc33635fffeaa74cf613e19a373e6cdf`.
- Exact comparison base: merged PR122 `72142acd9750fe32b1d0e1f70b8dd06e25ba7430`.
- Compiler: Ubuntu Clang 18.1.3; runtime: pinned ONNX Runtime 1.30.0; optimized build.
- Both generated-inference bundles were downloaded, matched against their Actions artifact SHA-256, and successfully replayed with the retained manifest digest.

## Runs and integrity

| Event | CI run | CPU reported by the runner | Collection |
|---|---|---|---|
| push | [39146759972](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39146759972) | Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz | complete; no missing evidence |
| PR | [39146796328](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39146796328) | AMD EPYC 7763 64-Core Processor | complete; no missing evidence |

Each head/base comparison uses the same runner and fixture within that run. The two runs use different CPU models; do not pool their absolute times or treat them as repeated controlled sessions.

- push artifact: [`runtime-profile-39146759972-1`](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39146759972/artifacts/12143926496).
  ZIP SHA-256: `b7638cd5c0edf843229e3e02d75ae57f52439d738f97d35e6ac1374978d3aecc`.
  Generated-inference manifest SHA-256: `14f08cfeab147bf1cdab96c8280b9a50b78292510f5fe37d358bc99a47e03ffa`.
- PR artifact: [`runtime-profile-39146796328-1`](https://github.com/amarbanerjee23/ShortHand_Compiler/actions/runs/39146796328/artifacts/12143354756).
  ZIP SHA-256: `6b4ab77cd6653d195fca25ed42df12fc4fe0a188ff50487bd7231b10e85f6889`.
  Generated-inference manifest SHA-256: `dead981a163618fe09f4eed1cf1cbcd67e0fe13a8707735a8bdf8cc728343691`.

The complete bundles contain raw observations, generated source/object, binaries, model, commands, hashes, environment, diagnostics and test logs. Actions retention expires on **2027-01-16**. This committed summary outlives that retention; it is not a replacement for the raw bundles. Archive the full bundles durably before relying on them for a long-lived claim.

## Compiled FP32 probe

Synthetic zero-input MatMul+Add; one functional unit is one numerically verified ten-score output vector. Each cell/runner has 16 timed blocks after warmup, with 256 calls per block. Head and base link the same generated object to their respective runtime archives; compiler changes are not compared. The timed compiled entry includes zero initialization, registration, inference, validation, telemetry/logging and output verification. The main series queries last-call JSON after timing.

Direct ORT controls use the same model, one intra/inter thread, sequential execution, disabled spinning, preallocated buffers and independent output verification. BASIC and ALL are explicit configurations; ALL is not assumed globally optimal. The narrower direct boundary omits ShortHand registration/runtime policy work. Ratios are boundary comparisons, not proof of a language-only cost.

| Run | Cell | Head ms/vector | Base ms/vector | Head/base | Head/ORT BASIC | Head/ORT ALL |
|---|---|---:|---:|---:|---:|---:|
| push | b1-t1 | 0.021308 | 0.024654 | 0.8643 | 13.6491 | 13.5042 |
| push | b16-t1 | 0.001749 | 0.001988 | 0.8797 | 9.0024 | 8.8803 |
| push | b32-t1 | 0.000943 | 0.001054 | 0.8946 | 6.5493 | 6.6536 |
| PR | b1-t1 | 0.036151 | 0.039767 | 0.9091 | 21.6230 | 21.4247 |
| PR | b16-t1 | 0.002434 | 0.002677 | 0.9091 | 13.6682 | 13.7748 |
| PR | b32-t1 | 0.001244 | 0.001360 | 0.9146 | 9.6758 | 9.6031 |

Head/base is the ratio of mean block times; below 1 means a lower observed head time. The observed differences were 10.54–13.57% on the Intel push runner and 8.54–9.09% on the AMD PR runner. Both direct prepared ORT configurations remained faster. These synthetic zero-input observations do not measure held-out classification quality or an end-to-end service.

## JSON exporters consumed on every inference

This separate ABBA head/base series consumes telemetry, bridge request, observability and OTLP JSON inside every timed call. It exposes deferred formatting cost and is never combined with the main or direct-ORT series.

| Run | Cell | Head ms/vector | Head/base with observers |
|---|---|---:|---:|
| push | b1-t1 | 0.027233 | 0.9742 |
| push | b16-t1 | 0.002218 | 0.9948 |
| push | b32-t1 | 0.001170 | 0.9852 |
| PR | b1-t1 | 0.044656 | 0.9713 |
| PR | b16-t1 | 0.002833 | 0.9651 |
| PR | b32-t1 | 0.001463 | 0.9731 |

The differences shrink to 0.52–3.49% across these cells and runs. This is consistent with deferred bridge formatting contributing to the main-series result; the benchmark does not independently isolate the input-copy change from the formatting change. Do not advertise the main-series percentage for callers that read every exporter after every inference.

## Resident real-digit control

The separate resident application checks all 1,797 held-out Optdigits images for prediction, top-k, numerical and quality parity. Each cell has six balanced paired blocks. The paired ratio is the median of within-block head/base ratios, not the ratio of the displayed marginal medians. Positive median reduction means lower observed head time; negative means regression.

| Run | Cell | Head median µs/image | Base median µs/image | Paired ratio median | Median reduction | Paired ratio range |
|---|---|---:|---:|---:|---:|---|
| push | b1-t1 | 2.961238 | 3.009118 | 0.9847 | 1.53% | 0.9234–1.0043 |
| push | b16-t1 | 0.664290 | 0.655615 | 1.0140 | -1.40% | 0.9888–1.0264 |
| push | b32-t1 | 0.558432 | 0.556353 | 1.0041 | -0.41% | 0.9907–1.0102 |
| push | b16-t2 | 0.664102 | 0.658276 | 0.9975 | 0.25% | 0.8436–1.0468 |
| push | b16-t4 | 0.667638 | 0.657019 | 1.0130 | -1.30% | 0.9964–1.0691 |
| PR | b1-t1 | 3.106900 | 3.171186 | 0.9880 | 1.20% | 0.9601–1.0601 |
| PR | b16-t1 | 0.555592 | 0.556672 | 0.9996 | 0.04% | 0.9941–1.0085 |
| PR | b32-t1 | 0.470617 | 0.472240 | 0.9997 | 0.03% | 0.9835–1.0069 |
| PR | b16-t2 | 0.558292 | 0.559192 | 0.9973 | 0.27% | 0.9819–1.0076 |
| PR | b16-t4 | 0.558307 | 0.563983 | 0.9909 | 0.91% | 0.9321–1.0009 |

Changes are small and mixed, and every cell in both runs contains paired ratios on both sides of 1. These controls do not establish a resident-application performance improvement. Ranges are observed minima/maxima, not confidence intervals.

## Energy and claim status

Both runs found no readable RAPL or AMD package/socket energy domains and no matching E2/E3 profiles. `highest_available_evidence_class` is null, `physical_system_energy_measured` is false, and component-energy comparison is unavailable. Unavailable joules remain null; time, TDP and utilization are not converted into energy. No energy-saving, lower-power, lowest-carbon, certification or production-scope claim is supported.

See [the implementation and evidence roadmap](../post_pr122_energy_implementation_plan.md) for same-artifact controls, calibrated measurement boundaries, quality constraints, independent confirmation, uncertainty and scoped claim criteria.

## Correctness, CI and installers

- Push CI: all 21 jobs successful, including publisher. PR CI: all 20 applicable jobs successful; the push-only publisher correctly skipped. Tooling and experiment-results workflows also passed for both events.
- Hosted native package lifecycle gates passed for Linux x64, Linux ARM64, Windows x64 and macOS ARM64. Full compiler/MLIR, toolchain, security, sanitizer, ABI and runtime evidence gates passed.
- Both retained evidence indexes report complete collection with no missing items. Generated outputs passed the numerical oracle; resident outputs passed prediction/top-k/numerical/quality equivalence.
- [Development release](https://github.com/amarbanerjee23/ShortHand_Compiler/releases/tag/build-39146759972-1-ca62eca1dc33): 37 assets including four native installers. The tag resolves to the measured runtime revision above. Before publication, the publisher verified candidate/lifecycle receipts and downloaded every uploaded asset for exact-set and SHA-256 verification.
- These are unsigned development installers. Existing platform opt-in requirements, default production allowlist and GA restrictions remain in force.

## Replay

Download and verify the relevant artifact ZIP against its SHA-256 above, then extract it as `PROFILE`. Check out the measured runtime revision. The push bundle can be replayed with:

```sh
python3 experiments/energy/generated_infer_evidence.py \
  --output "$PROFILE/generated-infer" \
  --replay-sha256 14f08cfeab147bf1cdab96c8280b9a50b78292510f5fe37d358bc99a47e03ffa
```

For the PR bundle use manifest digest `dead981a163618fe09f4eed1cf1cbcd67e0fe13a8707735a8bdf8cc728343691`. Replay checks retained hashes and validates main/observer matrices before regenerating `GENERATED.md` and `report.json`. The full collection commands are in [runtime-profile.yml](../../.github/workflows/runtime-profile.yml).
