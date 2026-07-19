#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${1:-${ROOT_DIR}/dist/release}"
mkdir -p "${OUT_DIR}"

cd "${ROOT_DIR}"
COMMIT_SHA="$(git rev-parse HEAD 2>/dev/null || echo unknown)"
SHORT_SHA="${COMMIT_SHA:0:12}"
GENERATED_AT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
TRACKED_COUNT="$(git ls-files | wc -l | tr -d ' ')"

HASH_MANIFEST="${OUT_DIR}/source_files.sha256"
if command -v sha256sum >/dev/null 2>&1; then
  git ls-files | sort | while IFS= read -r file; do
    if [[ -f "${file}" ]]; then
      sha256sum "${file}"
    fi
  done > "${HASH_MANIFEST}"
else
  : > "${HASH_MANIFEST}"
fi

cat > "${OUT_DIR}/sbom.spdx.json" <<JSON
{
  "spdxVersion": "SPDX-2.3",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "name": "ShortHand_Compiler candidate source SBOM",
  "documentNamespace": "https://github.com/amarbanerjee23/ShortHand_Compiler/sbom/${COMMIT_SHA}",
  "creationInfo": {
    "created": "${GENERATED_AT}",
    "creators": ["Tool: scripts/generate_release_sbom.sh"]
  },
  "packages": [
    {
      "name": "ShortHand compiler source",
      "SPDXID": "SPDXRef-Package-ShortHandCompilerSource",
      "versionInfo": "${SHORT_SHA}",
      "downloadLocation": "https://github.com/amarbanerjee23/ShortHand_Compiler",
      "filesAnalyzed": true,
      "licenseConcluded": "NOASSERTION",
      "licenseDeclared": "NOASSERTION",
      "copyrightText": "NOASSERTION",
      "externalRefs": [
        {
          "referenceCategory": "PACKAGE-MANAGER",
          "referenceType": "purl",
          "referenceLocator": "pkg:github/amarbanerjee23/ShortHand_Compiler@${COMMIT_SHA}"
        }
      ]
    },
    {
      "name": "ShortHand optional AI SDK integrations",
      "SPDXID": "SPDXRef-Package-OptionalAISDKIntegrations",
      "versionInfo": "optional-sdk-roots-not-vendored",
      "downloadLocation": "NOASSERTION",
      "filesAnalyzed": false,
      "licenseConcluded": "NOASSERTION",
      "licenseDeclared": "NOASSERTION",
      "copyrightText": "NOASSERTION"
    }
  ],
  "relationships": [
    {
      "spdxElementId": "SPDXRef-DOCUMENT",
      "relationshipType": "DESCRIBES",
      "relatedSpdxElement": "SPDXRef-Package-ShortHandCompilerSource"
    }
  ]
}
JSON

cat > "${OUT_DIR}/release_provenance.json" <<JSON
{
  "schema_version": "shorthand.release.provenance.v1",
  "repository": "amarbanerjee23/ShortHand_Compiler",
  "commit": "${COMMIT_SHA}",
  "generated_at": "${GENERATED_AT}",
  "tracked_file_count": ${TRACKED_COUNT},
  "source_hash_manifest": "source_files.sha256",
  "sbom": "sbom.spdx.json",
  "release_status": "candidate_evidence_only",
  "attestation_status": "unsigned_local_candidate",
  "certification_status": "not_certified_by_external_authority",
  "notes": [
    "Generated with repository-local dependency-free tooling.",
    "Optional AI SDK binaries are not vendored in this repository.",
    "This provenance file is candidate release evidence and not a cryptographic signature."
  ]
}
JSON

cat > "${OUT_DIR}/release_manifest.txt" <<EOF
ShortHand candidate release evidence
commit=${COMMIT_SHA}
generated_at=${GENERATED_AT}
sbom=sbom.spdx.json
provenance=release_provenance.json
hashes=source_files.sha256
status=candidate_evidence_only
EOF

printf 'Generated release evidence in %s\n' "${OUT_DIR}"
