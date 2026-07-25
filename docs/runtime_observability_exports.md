# Runtime observability exports

## Purpose

This document describes the dependency-free runtime observability export surface for the ShortHand runtime hook library.

The current status is:

`runtime_observability_export_status: dependency_free_prometheus_and_otlp_like_exports`

This means the runtime can expose its in-memory hook counters and latest inference telemetry in export-friendly text/JSON formats without linking a Prometheus client, OTLP SDK, collector, or network transport.

## Public C ABI

The runtime hook library exposes two observability export functions:

```c
const char *short_runtime_prometheus_metrics(void);
const char *short_runtime_otlp_spans_json(void);
```

Both functions return pointers to runtime-owned cached strings. Callers should copy the returned string if they need to retain it across later runtime calls.

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

## OTLP-like span JSON

`short_runtime_otlp_spans_json()` returns a JSON document with schema:

`shorthand.runtime.otlp_spans.v1`

The document contains:

- a resource block with `service.name: shorthand-runtime`,
- a single latest-infer span named `short_ai_infer`,
- runtime counters as span attributes,
- the latest inference telemetry JSON,
- the latest compiled/typed infer bridge request JSON.

This is an OTLP-like interchange payload, not a direct OTLP SDK exporter.

## Boundary

This PR does not add a network exporter, collector integration, daemon, scrape endpoint, or OpenTelemetry SDK dependency. It only gives embedders stable strings that can be served by a host process or converted into a real exporter later.

Runtime observability implementation with real OTLP/Prometheus export remains broader production work. The new functions reduce that blocker by adding dependency-free export hooks inside the runtime library.

## Validation

The gate `scripts/check_runtime_observability_exports.sh` validates the ABI symbols, required documentation markers, and runs `tests/codegen/test_runtime_observability_exports.sh`.

The test builds against `libshorthand_runtime.a`, registers model/tensor/GreenAI records, runs a bridge-pending infer call, then validates the Prometheus-style metrics, OTLP-like span JSON, and existing observability JSON.
