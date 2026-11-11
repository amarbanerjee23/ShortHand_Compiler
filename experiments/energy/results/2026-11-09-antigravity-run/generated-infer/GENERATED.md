# Compiled FP32 observations

synthetic zero-input FP32 MatMul+Add; no classification accuracy claim

compiled .short entry including zero tensor initialization, registration, inference, runtime validation/telemetry/logging and independent output verification; direct ORT includes its prepared invocation and identical output verification; block wall clocks. The main series queries last-call JSON after timing; separate head/base observer controls consume all four JSON exporters on every timed call.

Descriptive shared-runner observations; no latency or energy claim is authorized.

Both direct controls use one intra/inter thread, sequential execution, spinning disabled, and preallocated input/output. ALL is an explicit tuning candidate, not a claim of globally optimal tuning.

| Cell | Head ms/vector | Head/base | Head/ORT BASIC | Head/ORT ALL | ORT ALL/BASIC |
|---|---:|---:|---:|---:|---:|
| b1-t1 | 0.031740 | unavailable | 24.6393 | 24.4391 | 1.0082 |
| b16-t1 | 0.002150 | unavailable | 14.6093 | 14.7440 | 0.9909 |
| b32-t1 | 0.001097 | unavailable | 9.8382 | 9.2770 | 1.0605 |

## Diagnostic phase profile

Separate instrumented binary, 64 verified warm calls/cell. Exclusive phases sum to the instrumented total. Clock overhead is included, never subtracted. These are attribution observations, not latency or energy controls.

| Phase | b1 mean ns/call | b16 mean ns/call | b32 mean ns/call |
|---|---:|---:|---:|
| entry_residual | 454.1 | 486.3 | 513.0 |
| facade_lock | 137.4 | 139.8 | 141.3 |
| registration | 386.9 | 402.5 | 383.9 |
| bridge_validation | 183.9 | 198.0 | 199.7 |
| descriptor_input | 110.1 | 85.1 | 106.3 |
| policy_refresh | 255.2 | 268.2 | 265.4 |
| hardware_probe | 7219.9 | 7417.2 | 7694.3 |
| routing | 33.5 | 39.8 | 37.8 |
| runtime_validation | 409.3 | 384.1 | 387.4 |
| snapshot | 16627.7 | 16562.0 | 17293.5 |
| session_prepare | 0.0 | 0.0 | 0.0 |
| backend_other | 1778.0 | 2286.1 | 2758.6 |
| ort_run | 2685.3 | 2758.2 | 3299.6 |
| runtime_telemetry | 1719.0 | 2017.8 | 1772.2 |
| bridge_output | 49.6 | 51.9 | 60.4 |
| bridge_telemetry_log | 372.4 | 365.5 | 390.8 |
| oracle | 81.2 | 1033.3 | 1689.9 |

## Diagnostic performance events

calling thread user-space events only; other threads and kernel excluded; independent counter windows include observer work; cold and warm boundaries separate; empty observer control retained without subtraction; cache_misses is not an LLC metric

Separate executable and executions; no PMU counts enter the latency or energy controls. Raw counts, event IDs, enabled/running times, permission errors and unavailable fields are retained in `events.json`. No multiplexed estimate or joules conversion is made.


Energy is recorded separately in each `energy-*.energy.json`; unavailable counters produce null joules.
b16-t2 and b16-t4 are unsupported for compiled hooks; all five real-digit cells remain in resident evidence.
These are zero-input inference vectors, not held-out digit accuracy or end-to-end serving measurements.
