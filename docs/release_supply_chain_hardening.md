# Release supply-chain hardening

## Purpose

This document defines the first release evidence gate for ShortHand. The goal is to make release candidates easier to audit without claiming that the project has completed all production release controls.

## What this PR adds

PR #39 adds dependency-free candidate release evidence generation:

- `scripts/generate_release_sbom.sh` creates:
  - `sbom.spdx.json`,
  - `release_provenance.json`,
  - `source_files.sha256`,
  - `release_manifest.txt`.
- `schemas/release/release_provenance.schema.json` defines the expected provenance shape.
- `scripts/check_release_supply_chain.sh` validates the generated files and performs a baseline source scan.

## Evidence status

The generated files are candidate release evidence only. They are not a signed attestation and do not replace independent release review.

The current provenance output explicitly records:

- `release_status: candidate_evidence_only`,
- `attestation_status: unsigned_local_candidate`,
- `certification_status: not_certified_by_external_authority`.

## Optional SDK boundary

The repository does not vendor optional AI SDK binaries. Optional SDK roots such as ONNX Runtime, LibTorch, TensorRT, OpenVINO, llama.cpp, OpenBLAS, and Eigen remain externally supplied. The candidate SBOM records this boundary rather than pretending those SDKs are part of the source tree.

## Baseline security scan

The release supply-chain gate checks for common accidental secret patterns, including private key headers, GitHub personal access token patterns, and AWS access key IDs. This is a baseline repository scan, not a substitute for a full secret-scanning product.

## Remaining release hardening work

The following items remain open:

1. Generate a richer SBOM with a dedicated tool such as Syft when the release environment supports it.
2. Add signed artifact attestations using a signing workflow such as Sigstore/Cosign or a comparable enterprise signing system.
3. Add dependency vulnerability scanning with a tool such as Trivy, Grype, OSV-Scanner, or the organization-approved scanner.
4. Publish release artifacts from a protected workflow rather than a local candidate directory.
5. Add policy checks for release tags, provenance retention, and reviewer approval.

## Claim boundary

This repository still must not claim official C3-ECO certification, carbon-neutral operation, zero-carbon execution, or full enterprise release readiness unless those claims are backed by the required evidence and external authority where applicable.
