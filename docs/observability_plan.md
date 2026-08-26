# Observability Status and Production Plan

observability_status_version: 2026-08-23-pr87
current_status: implemented_process_scoped_serving_v1
production_claim: false

The runtime exposes bounded JSON snapshots, Prometheus-format metrics and OTLP-shaped span JSON for inference totals, success/not-executed/error state, backend/reason, latency and bridge context. A loopback HTTP adapter serves Prometheus metrics. Candidate evidence records execution and measurement availability without fabricating energy values.

PR87 adds versioned serving health JSON and low-cardinality Prometheus metrics tied to bounded concurrency, queue saturation, deadline, cancellation and drain state. The serving instance enforces one tenant scope per process; tenant and request identifiers never become metric labels. Fault/load/soak, sanitizer, TSan and worker lifecycle evidence exercise these signals.

This is not a public network service claim. The legacy loopback adapter is not hardened public ingress, and authentication, authorization and TLS remain host responsibilities. PR89 and PR95 own measurement provenance, energy sensors, uncertainty and performance/energy regression telemetry.
