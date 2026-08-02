# OTLP exporter adapter

## Contract status

- `otlp_exporter_contract_version: 1.0.0`
- `otlp_exporter_status: bounded_one_shot_otlp_http_trace_delivery`
- `source_snapshot_schema: shorthand.runtime.otlp_spans.v1`
- `transport: OTLP/HTTP JSON over HTTP/1.1`
- `default_endpoint: http://127.0.0.1:4318/v1/traces`
- `runtime_abi_change: none`
- `runtime_external_symbol_count: 25`
- `delivery_claim_boundary: http_acceptance_is_not_end_to_end_trace_storage`

## Purpose

PR63 adds an optional installed host executable named `shorthand_otlp_exporter`. It converts the dependency-free runtime snapshot returned by `short_runtime_otlp_spans_json()` into an OTLP/HTTP JSON `ExportTraceServiceRequest` and sends one bounded request to a collector.

The adapter is separate from the frozen runtime ABI. No new `short_*` function is introduced, and the runtime public symbol manifest remains exactly 25 symbols.

## Why the adapter accepts file and stdin input

Runtime counters and the latest inference evidence are process-local. A separate exporter process cannot read another ShortHand application's in-memory runtime state.

The exporter therefore supports three explicit source modes:

1. No input option: export the adapter process's local `short_runtime_otlp_spans_json()` snapshot.
2. `--input-file PATH`: export a bounded snapshot that the ShortHand application wrote to a file.
3. `--stdin`: export a bounded snapshot supplied through a pipe.

File and stdin handoff let an application or supervisor export the actual application snapshot without expanding the runtime ABI or pretending that process-local state is globally shared. Producers should write files atomically before invoking the exporter.

## OTLP request shape

The exporter posts JSON to `/v1/traces` by default and emits a standards-shaped OTLP trace request containing:

- `resourceSpans`,
- the configured `service.name`,
- exporter and service version attributes,
- a `shorthand.runtime.snapshot` span,
- the original `shorthand.runtime.otlp_spans.v1` document as a string attribute,
- source metadata identifying runtime, file, or stdin handoff.

The original runtime document is retained as evidence rather than loosely reinterpreting every custom field as a native OpenTelemetry field. A later schema-evolution PR may map more runtime attributes into first-class OTLP attributes while preserving backward compatibility.

## Delivery and retry contract

The executable exits successfully only when:

- dry-run generation completed, with `delivered=false`, or
- the collector returned an HTTP 2xx status, with `delivered=true`.

It retries only failures that can reasonably be transient:

- connection, send, or response failures,
- HTTP 408,
- HTTP 429,
- HTTP 5xx.

Other HTTP 4xx responses are permanent failures and are not retried. Retry count is bounded from 1 to 10 and exponential backoff is capped at 60 seconds. Connect, send, receive, source snapshot, and collector response sizes are bounded.

Representative status lines are:

```text
OTLP_EXPORT_DELIVERY status=delivered delivered=true attempts=2 http_status=200 source=file
OTLP_EXPORT_DELIVERY status=failed delivered=false attempts=1 http_status=400 reason=http_rejected source=file
OTLP_EXPORT_DELIVERY status=dry_run delivered=false attempts=0 source=runtime
```

HTTP 2xx proves that the configured endpoint accepted the request at the transport layer. It does not prove collector processing, backend persistence, retention, indexing, or query availability.

## Authentication and transport boundary

An optional Authorization header can be read from an environment variable using `--authorization-env NAME`. The secret is not accepted as a command-line value and is never printed. Newlines and oversized header values are rejected.

This release implements plain OTLP/HTTP. It does not implement native TLS, mTLS, certificate management, proxy negotiation, gzip, or gRPC. For non-loopback or production deployment, place the exporter behind a local OpenTelemetry Collector, service mesh, or approved TLS proxy and apply network policy and secret management.

## Usage

Export the adapter process's local runtime snapshot:

```bash
shorthand_otlp_exporter
```

Export a snapshot written by the application:

```bash
shorthand_otlp_exporter \
  --host 127.0.0.1 \
  --port 4318 \
  --path /v1/traces \
  --input-file /run/shorthand/runtime-otlp.json
```

Use an authorization value from the environment:

```bash
export SHORTHAND_OTLP_AUTH='Bearer <token>'
shorthand_otlp_exporter \
  --input-file /run/shorthand/runtime-otlp.json \
  --authorization-env SHORTHAND_OTLP_AUTH
```

Inspect generated OTLP JSON without claiming delivery:

```bash
shorthand_otlp_exporter --dry-run
```

## Validation

`scripts/check_otlp_exporter_adapter.sh` runs a real loopback collector fixture and verifies:

- successful OTLP/HTTP delivery,
- request path, content type, authorization header, and payload markers,
- retry after HTTP 503 followed by HTTP 200,
- no retry after permanent HTTP 400,
- invalid snapshot rejection before delivery,
- missing authorization environment rejection,
- installation of the executable,
- no change to the frozen runtime ABI.

## Production boundary

The adapter closes the basic OTLP collector-delivery gap, but ShortHand remains controlled beta. This PR does not provide continuous batching, disk queues, collector discovery, TLS, multi-tenant isolation, sampling policy, or end-to-end trace storage guarantees.
