#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${1:-}"
PLATFORM="${2:-}"
STAGE_DIR="${3:-}"
OUT_DIR="${4:-}"

if [[ -z "${VERSION}" || -z "${PLATFORM}" || -z "${STAGE_DIR}" || -z "${OUT_DIR}" ]]; then
  echo "usage: $0 <version> <platform> <installed-stage-dir> <output-dir>" >&2
  exit 2
fi

bash "${ROOT_DIR}/scripts/check_release_version_policy.sh" "${VERSION}" >/dev/null
[[ "${PLATFORM}" =~ ^(linux-x64|linux-arm64|macos-arm64|windows-x64)$ ]] || {
  echo "error: unsupported release platform id: ${PLATFORM}" >&2
  exit 1
}
[[ -d "${STAGE_DIR}" ]] || { echo "error: installed stage directory does not exist: ${STAGE_DIR}" >&2; exit 1; }
[[ -d "${STAGE_DIR}/bin" ]] || { echo "error: installed stage has no bin directory" >&2; exit 1; }

compiler="${STAGE_DIR}/bin/short_hand"
green_tool="${STAGE_DIR}/bin/green_ai_tool"
if [[ "${PLATFORM}" == "windows-x64" ]]; then
  compiler="${compiler}.exe"
  green_tool="${green_tool}.exe"
fi
[[ -s "${compiler}" ]] || { echo "error: staged compiler binary missing: ${compiler}" >&2; exit 1; }
[[ -s "${green_tool}" ]] || { echo "error: staged Green AI tool missing: ${green_tool}" >&2; exit 1; }

mkdir -p "${OUT_DIR}"
OUT_DIR="$(cd "${OUT_DIR}" && pwd)"
STAGE_DIR="$(cd "${STAGE_DIR}" && pwd)"
version_no_v="${VERSION#v}"
stem="shorthand-${version_no_v}-${PLATFORM}"
archive="${OUT_DIR}/${stem}.tar"
sbom="${OUT_DIR}/${stem}.spdx.json"
provenance="${OUT_DIR}/${stem}.provenance.json"
checksums="${OUT_DIR}/${stem}.SHA256SUMS"
manifest="${OUT_DIR}/${stem}.manifest"

rm -f "${archive}" "${sbom}" "${provenance}" "${checksums}" "${manifest}"
(
  cd "${STAGE_DIR}"
  LC_ALL=C cmake -E tar cf "${archive}" --format=gnutar .
)
archive_sha="$(cmake -E sha256sum "${archive}" | awk '{print $1}')"
commit_sha="$(git -C "${ROOT_DIR}" rev-parse HEAD)"
generated_at="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

cat > "${sbom}" <<JSON
{
  "spdxVersion": "SPDX-2.3",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "name": "ShortHand ${VERSION} ${PLATFORM} release artifact SBOM",
  "documentNamespace": "https://github.com/amarbanerjee23/ShortHand_Compiler/releases/${VERSION}/${PLATFORM}/${archive_sha}",
  "creationInfo": {
    "created": "${generated_at}",
    "creators": ["Tool: scripts/prepare_release_bundle.sh"]
  },
  "packages": [
    {
      "name": "${stem}.tar",
      "SPDXID": "SPDXRef-Package-ShortHand-${PLATFORM}",
      "versionInfo": "${version_no_v}",
      "downloadLocation": "NOASSERTION",
      "filesAnalyzed": false,
      "checksums": [{"algorithm": "SHA256", "checksumValue": "${archive_sha}"}],
      "licenseConcluded": "NOASSERTION",
      "licenseDeclared": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }
  ],
  "relationships": [{
    "spdxElementId": "SPDXRef-DOCUMENT",
    "relationshipType": "DESCRIBES",
    "relatedSpdxElement": "SPDXRef-Package-ShortHand-${PLATFORM}"
  }]
}
JSON

cat > "${provenance}" <<JSON
{
  "schema_version": "shorthand.release.bundle_provenance.v2",
  "repository": "amarbanerjee23/ShortHand_Compiler",
  "commit": "${commit_sha}",
  "version": "${VERSION}",
  "platform": "${PLATFORM}",
  "artifact": "${stem}.tar",
  "artifact_sha256": "${archive_sha}",
  "sbom": "${stem}.spdx.json",
  "release_status": "unsigned_candidate",
  "attestation_status": "pending_oidc_attestation",
  "production_claim": false,
  "generated_at": "${generated_at}"
}
JSON

{
  cmake -E sha256sum "${archive}"
  cmake -E sha256sum "${sbom}"
  cmake -E sha256sum "${provenance}"
} | sed "s#  ${OUT_DIR}/#  #" > "${checksums}"

cat > "${manifest}" <<EOF
schema=shorthand.release.bundle.v2
version=${VERSION}
platform=${PLATFORM}
commit=${commit_sha}
artifact=${stem}.tar
artifact_sha256=${archive_sha}
sbom=${stem}.spdx.json
provenance=${stem}.provenance.json
checksums=${stem}.SHA256SUMS
release_status=unsigned_candidate
production_claim=false
EOF

printf 'PREPARED_RELEASE_BUNDLE version=%s platform=%s artifact=%s sha256=%s\n' "${VERSION}" "${PLATFORM}" "${archive}" "${archive_sha}"
