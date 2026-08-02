# Prometheus scrape endpoint host adapter

prometheus_scrape_adapter_contract_version: 1.0.0
prometheus_scrape_adapter_status: loopback_default_bounded_http_metrics_host
prometheus_metrics_source: frozen_runtime_short_runtime_prometheus_metrics
runtime_abi_change: none
runtime_external_symbol_count: 25
production_claim_boundary: scrape_adapter_is_not_hardened_public_ingress

## Purpose

The ShortHand runtime already renders dependency-free Prometheus text through `short_runtime_prometheus_metrics()`. The `shorthand_prometheus_adapter` executable provides the missing host boundary by serving that text over a small HTTP/1.1 endpoint without adding network functions to the frozen runtime ABI.

The adapter is operational glue around the installed runtime. It is not part of the compiler grammar and it does not change inference, backend selection, hardware readiness, certification evidence or runtime success semantics.

## Endpoints

| Request | Response |
| --- | --- |
| `GET /metrics` | `200 OK`, Prometheus text format `0.0.4` from the current process-wide runtime context |
| `GET /healthz` | `200 OK`, body `ok` |
| other `GET` paths | `404 Not Found` |
| non-`GET` methods | `405 Method Not Allowed` with `Allow: GET` |
| malformed requests | `400 Bad Request` |
| oversized headers | `431 Request Header Fields Too Large` |

Responses include an explicit content length, `Connection: close`, `Cache-Control: no-store` and `X-Content-Type-Options: nosniff`.

## Safe defaults and bounded behavior

The default listener is `127.0.0.1:9464`. Binding to another IPv4 address prints a warning because the adapter does not implement TLS, authentication, authorization or tenant isolation.

The host adapter provides:

1. an 8 KiB default request-header limit,
2. a two-second default socket read/write timeout,
3. a bounded listen backlog,
4. one request per connection,
5. graceful `SIGINT` and `SIGTERM` handling,
6. an optional maximum-request count for deterministic tests and supervised jobs,
7. an ephemeral-port mode using `--port 0`.

The adapter deliberately supports only a small HTTP subset. It must be placed behind an authenticated reverse proxy or service-mesh ingress before any non-loopback exposure.

## Usage

```text
shorthand_prometheus_adapter --listen 127.0.0.1 --port 9464
```

Available controls:

```text
--listen ADDRESS
--port PORT
--max-requests COUNT
--read-timeout-ms MILLISECONDS
--request-limit-bytes BYTES
```

On successful startup the process prints a machine-readable readiness line:

```text
PROMETHEUS_ADAPTER_LISTENING host=127.0.0.1 port=9464 metrics_path=/metrics health_path=/healthz
```

## Runtime and ABI boundary

The adapter links to the existing `shorthand_runtime` target and calls only the frozen public function `short_runtime_prometheus_metrics()`. It introduces no new `short_*` symbol. The ABI stability gate must continue to observe exactly 25 public runtime symbols.

Metrics describe only the adapter process's process-wide runtime context. The ABI v1 process isolation policy still applies. Running one adapter does not aggregate metrics from other ShortHand processes.

## Validation evidence

`tests/operations/test_prometheus_scrape_adapter.sh` performs a clean CMake build, starts the adapter on an ephemeral loopback port, sends real socket requests, validates status codes and headers, verifies representative ShortHand metrics, checks deterministic shutdown, installs the executable and re-runs the frozen ABI gate.

`scripts/check_prometheus_scrape_adapter.sh` guards the implementation, documentation, CMake integration and test evidence.

## Explicit non-claims

This adapter does not provide:

- TLS termination,
- authentication or authorization,
- multi-tenant isolation,
- rate limiting across clients,
- HTTP/2,
- distributed metric aggregation,
- Kubernetes ServiceMonitor resources,
- a public-internet-ready ingress,
- complete enterprise production readiness.

Those deployment controls remain separate from this host adapter and from the frozen runtime ABI.
