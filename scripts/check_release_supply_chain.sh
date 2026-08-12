#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

OUT_DIR="/tmp/shorthand_release_supply_chain"
rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required text: ${needle}" >&2
    exit 1
  fi
}

# Preserve the dependency-free unsigned source-evidence baseline introduced by
# the earlier release supply-chain PR. The protected release workflow builds on
# this evidence rather than silently deleting its old-task contract.
require_file SECURITY.md
require_file docs/release_supply_chain_hardening.md
require_file schemas/release/release_provenance.schema.json
require_file scripts/generate_release_sbom.sh
bash -n scripts/generate_release_sbom.sh
bash scripts/generate_release_sbom.sh "${OUT_DIR}" >/tmp/shorthand_generate_release_sbom.out 2>&1
require_contains "${OUT_DIR}/sbom.spdx.json" '"spdxVersion": "SPDX-2.3"'
require_contains "${OUT_DIR}/sbom.spdx.json" 'SPDXRef-Package-ShortHandCompilerSource'
require_contains "${OUT_DIR}/sbom.spdx.json" 'optional-sdk-roots-not-vendored'
require_contains "${OUT_DIR}/release_provenance.json" '"schema_version": "shorthand.release.provenance.v1"'
require_contains "${OUT_DIR}/release_provenance.json" '"release_status": "candidate_evidence_only"'
require_contains "${OUT_DIR}/release_provenance.json" '"attestation_status": "unsigned_local_candidate"'
require_contains "${OUT_DIR}/release_manifest.txt" 'status=candidate_evidence_only'
require_file "${OUT_DIR}/source_files.sha256"

# PR76 / roadmap PR75 adds the fail-closed protected publication contract.
require_file docs/signed_release_publication.md
require_file schemas/release/release_bundle_provenance.schema.json
require_file scripts/check_signed_release_contract.sh
bash -n scripts/check_signed_release_contract.sh
bash scripts/check_signed_release_contract.sh >/tmp/shorthand_signed_release_contract.out 2>&1 || {
  cat /tmp/shorthand_signed_release_contract.out >&2 || true
  exit 1
}

if git grep -n -I -E -e '-----BEGIN ((RSA|DSA|EC|OPENSSH) )?PRIVATE KEY-----|ghp_[A-Za-z0-9_]{20,}|AKIA[0-9A-Z]{16}' -- . >/tmp/shorthand_secret_scan.out 2>&1; then
  echo "error: potential secret material found" >&2
  cat /tmp/shorthand_secret_scan.out >&2
  exit 1
fi

if grep -R -n -I -E 'C3-ECO Certified|official_certification_granted[[:space:]]*:[[:space:]]*true|carbon neutral|zero-carbon' "${OUT_DIR}" >/tmp/shorthand_release_claim_scan.out 2>&1; then
  echo "error: unsupported release/certification claim found in generated release evidence" >&2
  cat /tmp/shorthand_release_claim_scan.out >&2
  exit 1
fi

printf 'PASS release supply-chain gate\n'
