# Signed release and protected publication

signed_release_contract_version: shorthand.release.protected.v1
roadmap_pr: PR75
github_pr: PR76
production_claim: false
publication_environment: production-release
publication_environment_status_at_pr_start: not_configured

## Objective

This contract upgrades ShortHand from unsigned candidate supply-chain evidence to a release workflow that can create cryptographically verifiable GitHub artifact attestations and publish only from immutable version tags. Signing proves artifact provenance and integrity. It does not prove functional correctness, external certification, deployment readiness, hardware qualification, or lower energy use than Python.

## Release paths

`workflow_dispatch` is a non-publishing dry run. It validates the requested version, builds the four PR74-qualified platform packages, generates checksums/SBOM/provenance and verifies the unsigned candidate bundles. It cannot enter the publication job.

A push of a tag matching `v*` is the only publication-capable event. `scripts/check_release_version_policy.sh` requires the tag to use `vMAJOR.MINOR.PATCH` or `vMAJOR.MINOR.PATCH-rc.N`, match the CMake project version and resolve to the exact checked-out commit.

The release workflow builds these qualified tiers:

- Linux x86-64 with Clang/LLVM 18,
- Linux arm64 with LLVM 18,
- macOS Apple Silicon with LLVM 18,
- Windows x64 with the UCRT64 Clang/LLVM toolchain qualified by roadmap PR74.

Every bundle contains the installed SDK plus `short_hand` and `green_ai_tool`, an archive SHA-256, an SPDX 2.3 artifact SBOM and `shorthand.release.bundle_provenance.v2` metadata. `scripts/verify_release_bundle.sh` binds the archive, SBOM and provenance to the same digest and rejects tampering.

## Privilege boundary

Normal `push` and `pull_request` CI retains read-only repository permissions except its existing status publication capability. The release workflow defaults to `contents: read`. Only the tag-only `publish` job receives:

- `contents: write`,
- `id-token: write`,
- `attestations: write`,
- `actions: read`.

That job is additionally bound to the GitHub Environment `production-release`.

## Required GitHub Environment configuration

Publication deliberately fails closed until a repository administrator configures `production-release` with all of the following:

1. at least one required reviewer,
2. prevent self-review enabled,
3. custom deployment branch/tag policies enabled,
4. generic protected-branch admission disabled for this release environment,
5. an exact deployment policy pattern `v*`.

The workflow reads the live Environment and deployment-policy REST resources immediately after environment admission and runs `scripts/check_protected_release_environment.sh`. An environment that exists but lacks any requirement above cannot sign or publish.

At the start of GitHub PR76, the repository did not have a `production-release` environment. This is an external repository-administration prerequisite, not a condition that the workflow is allowed to silently bypass. Until the environment is configured and an actual signed tag release is exercised, the compiler test matrix keeps signed protected release evidence at `partial` rather than overstating completion.

## Signing and verification

The security-sensitive actions in `.github/workflows/release.yml` are pinned to immutable commit SHAs. The publication job creates GitHub artifact build-provenance attestations and SPDX 2.3 SBOM attestations for every platform archive. It then runs `gh attestation verify` for both predicate classes, constrained to this repository, this release workflow and this source ref. Publication-mode bundle verification requires those successful verification records.

No private signing key is stored in this repository. The workflow depends on GitHub OIDC-backed artifact attestation rather than a long-lived repository signing secret.

## Draft publication and rollback

After cryptographic verification, the workflow creates a draft GitHub Release with `--verify-tag`. It downloads every staged asset and compares SHA-256 with the local qualified asset. Any failure after draft creation triggers deletion of the draft release while preserving the immutable git tag. Only after the downloaded assets match does the workflow switch the release out of draft state.

An already-existing release for the tag is rejected rather than overwritten.

## Negative evidence

Mandatory enterprise CI executes:

- valid/invalid release-version policy cases,
- branch-publication rejection,
- missing reviewer rejection,
- self-review-enabled rejection,
- missing custom `v*` policy rejection,
- checksum-tampered artifact rejection,
- unsigned candidate rejection in publication mode,
- static release-workflow anti-weakening checks for action pins, OIDC permissions, protected environment, attestation verification and rollback.

## Remaining blocker

The implementation is ready to exercise signed publication, but `TST017` remains partial until `production-release` is configured with the required protections and a real tag publication produces and verifies attestations. This is intentionally stricter than treating workflow source code as signing evidence.
