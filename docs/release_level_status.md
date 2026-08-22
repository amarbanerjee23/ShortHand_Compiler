# Release Level Status

release_level_status_version: 2026-08-22-pr86
current_maturity: controlled_beta
production_claim: false
current_github_pr: 86
final_planned_github_pr: 96

ShortHand is a controlled beta with a versioned `linux-x64-cpu-v1` backend qualification scope. It is not an enterprise production release, an external certification, or a general accelerator-support claim.

## Implemented release infrastructure

- deterministic multi-toolchain builds and installed consumers,
- frozen runtime ABI 1.0.0,
- mandatory sanitizer, fuzz and race gates,
- CodeQL, dependency, secret and license policy gates,
- hardened amd64/arm64 container and live Kubernetes qualification,
- SPDX 2.3 source and release-artifact SBOM generation,
- candidate provenance, signing workflow and rollback logic,
- runtime JSON, Prometheus and OTLP-shaped observability exports,
- live ONNX Runtime CPU numerical qualification for `linux-x64-cpu-v1`,
- machine-readable production truth and C3-ECO traceability.
- beta-0.4 exact float/string/typed-array execution and guarded type/memory descriptors.
- beta-0.5 expression calls, recursion, lexical cleanup, structured returns and safe label resolution.
- beta-0.6 enterprise ABI schemas, SHA-256 offline packages, SPDX dependency evidence and core FFI ABI 1.0.0.

## Open before enterprise production use

PR87-PR96 close concurrent serving, complete C3-ECO preparation, generated MLIR and composite execution lowering, representative workload, performance/energy and final release-candidate blockers. TST017 separately requires a real protected tag publication whose attestations verify cryptographically.

Only executed checks and retained artifacts are release evidence. Workflow source, a detected device, an installed SDK, a skipped test or a candidate evidence bundle is not proof of production readiness.
