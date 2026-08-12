# Release supply-chain hardening

## Purpose

This document defines the original dependency-free release evidence baseline introduced by PR40. That baseline remains part of the current release contract and is now complemented by `docs/signed_release_publication.md` for roadmap PR75 / GitHub PR76.

## PR40 baseline

`scripts/generate_release_sbom.sh` creates unsigned candidate source evidence:

- `sbom.spdx.json`,
- `release_provenance.json`,
- `source_files.sha256`,
- `release_manifest.txt`.

`schemas/release/release_provenance.schema.json` defines the historical v1 candidate provenance shape. These files remain **candidate release evidence only** and intentionally retain:

- `release_status: candidate_evidence_only`,
- `attestation_status: unsigned_local_candidate`,
- `certification_status: not_certified_by_external_authority`.

This v1 source evidence is not itself a signed attestation.

## Roadmap PR75 extension

GitHub PR76 adds a separate artifact-level protected publication contract rather than rewriting the historical v1 evidence into a false signed state. The new release path adds:

- immutable CMake/tag version validation,
- qualified Linux x64, Linux arm64, macOS arm64 and Windows x64 release bundles,
- SHA-256 binding,
- per-artifact SPDX 2.3 evidence,
- `shorthand.release.bundle_provenance.v2`,
- OIDC-backed GitHub build-provenance and SBOM attestations,
- `gh attestation verify` enforcement,
- a `production-release` environment protection check,
- draft-release asset re-verification and rollback.

The signed-release blocker remains partial until the repository's `production-release` environment is configured with the required protections and a real tag publication executes successfully. See `docs/signed_release_publication.md`.

## Optional SDK boundary

The repository does not vendor optional AI SDK binaries. Optional SDK roots such as ONNX Runtime, LibTorch, TensorRT, OpenVINO, llama.cpp, OpenBLAS and Eigen remain externally supplied. Release evidence records this boundary rather than pretending those SDKs are part of the repository-owned source or binary bundle.

## Baseline security scan

The release supply-chain gate still checks for common accidental secret patterns, including private key headers, GitHub personal access token patterns and AWS access key IDs. This baseline scan is not a substitute for the external vulnerability/SAST/license gate planned in roadmap PR76.

## Old-task compatibility

The historical phrase `Automated SBOM` remains part of the feature-tracker contract. The v1 candidate source evidence remains present so earlier release evidence is auditable while newer signing evidence is layered on top.

## Remaining release hardening work

1. Configure and exercise the protected `production-release` environment so TST017 can close with live signing evidence.
2. Add the external CVE, SAST, dependency and license policy gate in roadmap PR76.
3. Add container/Kubernetes production hardening in PR77.
4. Continue release-candidate blocker aggregation through PR86.

## Claim boundary

Cryptographic release provenance does not grant C3-ECO certification, prove carbon-neutral/zero-carbon execution, prove production deployment readiness, or prove lower energy use than Python. `production_claim: false` remains in force until the remaining roadmap evidence closes.
