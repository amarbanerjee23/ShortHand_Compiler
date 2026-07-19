# ShortHand AI Telemetry Schema

ShortHand runtime telemetry is emitted as structured JSON fragments that are safe to embed in evidence reports and can be translated to OTLP spans by deployment adapters.

The current implementation avoids adding a hard dependency on the OpenTelemetry C++ SDK to the default compiler build. This keeps no-SDK CI reproducible while preserving an OTLP-compatible attribute model for Kubernetes and enterprise observability integrations.

## Runtime telemetry record

| Field | Type | Meaning |
| --- | --- | --- |
| `component` | string | Runtime component, currently `shorthand.ai_runtime` |
| `backend` | string | Backend used, for example `onnxruntime_cpu` |
| `model` | string | Declared model name or model path |
| `status` | string | `success`, `backend_unavailable`, `invalid_input`, or `runtime_error` |
| `reason` | string | Human-readable reason or error code |
| `latency_ns` | integer | Wall-clock inference duration in nanoseconds |
| `input_elements` | integer | Number of input tensor elements |
| `output_elements` | integer | Number of output tensor elements |
| `measured_energy_available` | boolean | Whether direct energy measurement was available |
| `measured_energy_kwh` | number | Directly measured energy in kWh when available; otherwise `0` |

## OTLP-compatible attribute names

The helper `telemetryToOtlpLikeSpanJson` uses these attributes:

| Attribute | Meaning |
| --- | --- |
| `ai.system` | `shorthand` |
| `ai.backend` | Backend selected by runtime |
| `ai.model.name` | Model identifier |
| `ai.inference.status` | Inference status |
| `ai.inference.reason` | Failure/success reason |
| `ai.input.elements` | Input element count |
| `ai.output.elements` | Output element count |
| `ai.latency.ns` | Latency in nanoseconds |
| `ai.energy.measured` | Whether energy was directly measured |
| `ai.energy.kwh` | Measured kWh when available |

## Certification use

Telemetry can support C3-ECO candidate evidence only when it is tied to:

1. Product and version.
2. Functional unit.
3. Measurement boundary.
4. MQ/DQ classification.
5. Carbon factor and location/market basis.
6. Evidence-retention period.

Missing energy telemetry must be reported as unavailable. It must not be fabricated or replaced by offset claims.
