# Observability Status and Production Plan

observability_status_version: 2026-08-22-pr83
current_status: partial_dependency_free_exports
production_claim: false

The runtime exposes bounded JSON snapshots, Prometheus-format metrics and OTLP-shaped span JSON for inference totals, success/not-executed/error state, backend/reason, latency and bridge context. A loopback HTTP adapter serves Prometheus metrics. Candidate evidence records execution and measurement availability without fabricating energy values.

This is not yet a production serving observability claim. The runtime uses a serialized process-wide default context, the loopback adapter is not hardened public ingress, and multi-tenant isolation requires process boundaries. PR87 owns request correlation, deadlines, cancellation, bounded concurrency, backpressure, health/readiness, resource limits, tenant isolation, fault/load/soak evidence and operator runbooks. PR89 and PR95 own measurement provenance, energy sensors, uncertainty and performance/energy regression telemetry.
