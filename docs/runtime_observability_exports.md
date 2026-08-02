# Runtime observability exports

## Purpose

This document describes the dependency-free runtime observability export surface for the ShortHand runtime hook library and the optional host adapters that consume it.

The current status is:

`runtime_observability_export_status: dependency_free_exports_plus_optional_host_adapters`

The runtime library exposes in-memory hook counters and latest inference telemetry as stable text/JSON strings without linking a Prometheus client, OpenTelemetry SDK, collector, or network transport. Separate installed host executables can expose or deliver those strings without changing the frozen runtime ABI.

## Public C ABI

The runtime hook library exposes two observability export functions:

```c
const char *short_runtime_prometheus_metrics(void);
const char *short_runtime_otlp_spans_json(void);
```

Both functions return pointers to runtime-owned cached strings. Callers should copy the returned string if they need to retain it across later runtime calls.

The optional host adapters do not add public `short_*` symbols. Runtime ABI `1.0.0` remains frozen at exactly 25 public symbols.

## Prometheus-style metrics

`short_runtime_prometheus_metrics()` returns Prometheus-style metrics text with counters and gauges such as:

- `shorthand_runtime_models`
- `shorthand_runtime_tensors`
- `shorthand_runtime_contracts`
- `shorthand_runtime_measurements`
- `shorthand_runtime_infer_total`
- `shorthand_runtime_infer_success_total`
- `shorthand_runtime_infer_not_executed_total`
- `shorthand_runtime_infer_backend_unavailable_total`
- `shorthand_runtime_infer_invalid_input_total`
- `shorthand_runtime_last_infer_info`

The final metric carries the latest status, backend and reason as labels. Its value is always `1`.

The optional `shorthand_prometheus_adapter` serves these metrics through a bounded, loopback-default `/metrics` endpoint. See `docs/prometheus_scrape_host_adapter.md`.

## OTLP-like span JSON

`short_runtime_otlp_spans_json()` returns a JSON document with schema:

`shorthand.runtime.otlp_spans.v1`

The document contains:

- a resource block with `service.name: shorthand-runtime`,
- a single latest-infer span named `short_ai_infer`,
- runtime counters as span attributes,
- the latest inference telemetry JSON,
- the latest compiled/typed infer bridge request JSON.

This remains a dependency-free runtime interchange payload rather than a direct OpenTelemetry SDK object.

The optional `shorthand_otlp_exporter` converts this snapshot into an OTLP/HTTP JSON `resourceSpans` request and performs bounded one-shot collector delivery. It can consume the local process snapshot or a bounded file/stdin handoff from the actual ShortHand application. See `docs/otlp_exporter_adapter.md`.

## Boundary

The core runtime library remains network-free and does not depend on a Prometheus client, OpenTelemetry SDK, collector library, TLS library, or background exporter thread.

The host adapters are intentionally separate processes. Their state is process-local. A standalone adapter cannot inspect another application's runtime state unless that application provides an explicit file/stdin handoff. Prometheus and OTLP HTTP acceptance do not prove downstream persistence, retention, or query availability.

## Validation

The gate `scripts/check_runtime_observability_exports.sh` validates the ABI symbols, required documentation markers, and runs:

- `tests/codegen/test_runtime_observability_exports.sh`,
- `scripts/check_otlp_exporter_adapter.sh`.

The runtime test builds against `libshorthand_runtime.a`, registers model/tensor/GreenAI records, runs a bridge-pending infer call, and validates the Prometheus-style metrics, OTLP-like span JSON, and existing observability JSON.

The OTLP adapter gate starts a real loopback collector fixture and validates accepted delivery, retryable and permanent failure behavior, input and authorization validation, installation, and unchanged ABI.
