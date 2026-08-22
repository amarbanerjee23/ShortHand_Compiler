# SBOM Status and Release Contract

sbom_status_version: 2026-08-22-pr83
current_status: implemented_candidate_and_artifact_baseline
production_claim: false

`scripts/generate_release_sbom.sh` generates SPDX 2.3 evidence for the source tree and release artifacts. Release/security gates verify schema identity, source commit, toolchain/build context, artifact checksums, bundled dependency inventory and explicit optional-SDK boundaries.

SBOM generation does not prove that dependencies are vulnerability-free. The mandatory CodeQL, Trivy, dependency-delta, redistribution-license and security-exception controls remain separate fail-closed evidence. PR86 extends package/standard-library/FFI component provenance; PR91 binds certification evidence lineage; the protected tag exercise must retain the final SBOM with verified provenance and signatures.
