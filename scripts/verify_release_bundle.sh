#!/usr/bin/env bash
set -euo pipefail

BUNDLE_DIR="${1:-}"
MODE="${2:-}"
if [[ -z "${BUNDLE_DIR}" ]]; then
  echo "usage: $0 <bundle-dir> [--publication]" >&2
  exit 2
fi
[[ -d "${BUNDLE_DIR}" ]] || { echo "error: release bundle directory missing: ${BUNDLE_DIR}" >&2; exit 1; }
if [[ -n "${MODE}" && "${MODE}" != "--publication" ]]; then
  echo "error: unsupported verification mode: ${MODE}" >&2
  exit 2
fi

manifest="$(find "${BUNDLE_DIR}" -maxdepth 1 -type f -name 'shorthand-*.manifest' -print | head -n 1)"
[[ -n "${manifest}" && -s "${manifest}" ]] || { echo "error: release bundle manifest missing" >&2; exit 1; }
if [[ "$(find "${BUNDLE_DIR}" -maxdepth 1 -type f -name 'shorthand-*.manifest' -print | wc -l | tr -d ' ')" != "1" ]]; then
  echo "error: release bundle must contain exactly one manifest" >&2
  exit 1
fi

get_value() {
  local key="$1"
  sed -n "s/^${key}=//p" "${manifest}"
}

schema="$(get_value schema)"
version="$(get_value version)"
platform="$(get_value platform)"
commit="$(get_value commit)"
artifact_name="$(get_value artifact)"
artifact_sha="$(get_value artifact_sha256)"
sbom_name="$(get_value sbom)"
provenance_name="$(get_value provenance)"
checksums_name="$(get_value checksums)"
release_status="$(get_value release_status)"
production_claim="$(get_value production_claim)"

[[ "${schema}" == "shorthand.release.bundle.v2" ]] || { echo "error: unsupported release bundle schema" >&2; exit 1; }
[[ "${version}" =~ ^v[0-9]+\.[0-9]+\.[0-9]+(-rc\.[0-9]+)?$ ]] || { echo "error: invalid version in release bundle" >&2; exit 1; }
[[ "${platform}" =~ ^(linux-x64|linux-arm64|macos-arm64|windows-x64)$ ]] || { echo "error: invalid platform in release bundle" >&2; exit 1; }
[[ "${commit}" =~ ^[0-9a-f]{40}$ ]] || { echo "error: invalid commit digest in release bundle" >&2; exit 1; }
[[ "${artifact_sha}" =~ ^[0-9a-f]{64}$ ]] || { echo "error: invalid artifact SHA256 in release bundle" >&2; exit 1; }
[[ "${release_status}" == "unsigned_candidate" ]] || { echo "error: candidate manifest release status changed unexpectedly" >&2; exit 1; }
[[ "${production_claim}" == "false" ]] || { echo "error: release bundle cannot assert production readiness" >&2; exit 1; }

for name in "${artifact_name}" "${sbom_name}" "${provenance_name}" "${checksums_name}"; do
  [[ "${name}" == "$(basename "${name}")" && "${name}" != *'..'* ]] || {
    echo "error: unsafe release bundle file name: ${name}" >&2
    exit 1
  }
  [[ -s "${BUNDLE_DIR}/${name}" ]] || { echo "error: required release bundle file missing: ${name}" >&2; exit 1; }
done

actual_archive_sha="$(cmake -E sha256sum "${BUNDLE_DIR}/${artifact_name}" | awk '{print $1}')"
[[ "${actual_archive_sha}" == "${artifact_sha}" ]] || { echo "error: release artifact digest mismatch" >&2; exit 1; }

grep -Fq '"spdxVersion": "SPDX-2.3"' "${BUNDLE_DIR}/${sbom_name}" || { echo "error: artifact SBOM is not SPDX 2.3" >&2; exit 1; }
grep -Fq "\"checksumValue\": \"${artifact_sha}\"" "${BUNDLE_DIR}/${sbom_name}" || { echo "error: SBOM is not bound to the release artifact digest" >&2; exit 1; }
grep -Fq '"schema_version": "shorthand.release.bundle_provenance.v2"' "${BUNDLE_DIR}/${provenance_name}" || { echo "error: release provenance v2 marker missing" >&2; exit 1; }
grep -Fq "\"artifact_sha256\": \"${artifact_sha}\"" "${BUNDLE_DIR}/${provenance_name}" || { echo "error: release provenance is not bound to the artifact digest" >&2; exit 1; }
grep -Fq "\"commit\": \"${commit}\"" "${BUNDLE_DIR}/${provenance_name}" || { echo "error: release provenance commit mismatch" >&2; exit 1; }
grep -Fq '"production_claim": false' "${BUNDLE_DIR}/${provenance_name}" || { echo "error: release provenance claim boundary missing" >&2; exit 1; }

while read -r expected name; do
  [[ -n "${expected}" && -n "${name}" ]] || continue
  [[ "${name}" == "$(basename "${name}")" ]] || { echo "error: checksum manifest contains unsafe path" >&2; exit 1; }
  [[ -s "${BUNDLE_DIR}/${name}" ]] || { echo "error: checksum subject missing: ${name}" >&2; exit 1; }
  actual="$(cmake -E sha256sum "${BUNDLE_DIR}/${name}" | awk '{print $1}')"
  [[ "${actual}" == "${expected}" ]] || { echo "error: checksum mismatch for ${name}" >&2; exit 1; }
done < "${BUNDLE_DIR}/${checksums_name}"

if [[ "${MODE}" == "--publication" ]]; then
  provenance_attestation="${BUNDLE_DIR}/${artifact_name}.provenance.attestation.json"
  sbom_attestation="${BUNDLE_DIR}/${artifact_name}.sbom.attestation.json"
  for verified in "${provenance_attestation}" "${sbom_attestation}"; do
    [[ -s "${verified}" ]] || { echo "error: publication requires successful cryptographic attestation verification evidence: ${verified}" >&2; exit 1; }
    grep -Fq '"verificationResult"' "${verified}" || { echo "error: attestation verification result missing: ${verified}" >&2; exit 1; }
    grep -Fq "${artifact_sha}" "${verified}" || { echo "error: attestation verification is not bound to artifact SHA256" >&2; exit 1; }
  done
fi

printf 'PASS release bundle verification version=%s platform=%s mode=%s\n' "${version}" "${platform}" "${MODE:---candidate}"
