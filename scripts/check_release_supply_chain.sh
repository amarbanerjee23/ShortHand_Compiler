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

require_file SECURITY.md
require_file docs/release_supply_chain_hardening.md
require_file schemas/release/release_provenance.schema.json
require_file scripts/generate_release_sbom.sh

bash scripts/generate_release_sbom.sh "${OUT_DIR}" >/tmp/shorthand_generate_release_sbom.out 2>&1

require_contains "${OUT_DIR}/sbom.spdx.json" '"spdxVersion": "SPDX-2.3"'
require_contains "${OUT_DIR}/sbom.spdx.json" 'SPDXRef-Package-ShortHandCompilerSource'
require_contains "${OUT_DIR}/sbom.spdx.json" 'optional-sdk-roots-not-vendored'
require_contains "${OUT_DIR}/release_provenance.json" '"schema_version": "shorthand.release.provenance.v1"'
require_contains "${OUT_DIR}/release_provenance.json" '"release_status": "candidate_evidence_only"'
require_contains "${OUT_DIR}/release_provenance.json" '"attestation_status": "unsigned_local_candidate"'
require_contains "${OUT_DIR}/release_manifest.txt" 'status=candidate_evidence_only'
require_file "${OUT_DIR}/source_files.sha256"

if git grep -n -I -E '-----BEGIN ((RSA|DSA|EC|OPENSSH) )?PRIVATE KEY-----|ghp_[A-Za-z0-9_]{20,}|AKIA[0-9A-Z]{16}' -- . >/tmp/shorthand_secret_scan.out 2>&1; then
  echo "error: potential secret material found" >&2
  cat /tmp/shorthand_secret_scan.out >&2
  exit 1
fi

if git grep -n -I -E 'C3-ECO Certified|official_certification_granted[[:space:]]*:[[:space:]]*true|carbon neutral|zero-carbon' -- . ':!docs/c3eco_schema_and_claim_safety.md' ':!scripts/check_c3eco_claims_and_schema.sh' >/tmp/shorthand_release_claim_scan.out 2>&1; then
  echo "error: unsupported release/certification claim found" >&2
  cat /tmp/shorthand_release_claim_scan.out >&2
  exit 1
fi

printf 'PASS release supply-chain gate\n'
