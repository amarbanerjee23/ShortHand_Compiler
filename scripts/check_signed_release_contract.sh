#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

WORKFLOW=.github/workflows/release.yml
VERSION_POLICY=scripts/check_release_version_policy.sh
required_files=(
  "${WORKFLOW}"
  "${VERSION_POLICY}"
  scripts/prepare_release_bundle.sh
  scripts/verify_release_bundle.sh
  scripts/check_protected_release_environment.sh
  schemas/release/release_bundle_provenance.schema.json
  tests/release/test_release_version_policy.sh
  tests/release/test_release_bundle_tamper.sh
  tests/release/test_release_environment_policy.sh
  docs/signed_release_publication.md
)
for file in "${required_files[@]}"; do
  [[ -s "${file}" ]] || { echo "error: signed release contract file missing: ${file}" >&2; exit 1; }
done

for script in "${VERSION_POLICY}" scripts/prepare_release_bundle.sh scripts/verify_release_bundle.sh scripts/check_protected_release_environment.sh tests/release/*.sh; do
  bash -n "${script}"
done

require_workflow() {
  local needle="$1"
  grep -Fq -- "${needle}" "${WORKFLOW}" || { echo "error: release workflow missing required contract: ${needle}" >&2; exit 1; }
}
require_version_policy() {
  local needle="$1"
  grep -Fq -- "${needle}" "${VERSION_POLICY}" || { echo "error: release version policy missing required contract: ${needle}" >&2; exit 1; }
}

require_workflow 'workflow_dispatch:'
require_workflow 'tags:'
require_workflow "- 'v*'"
require_workflow 'cancel-in-progress: false'
require_workflow 'git fetch --no-tags origin +refs/heads/master:refs/remotes/origin/master'
require_workflow 'release-policy-preflight:'
require_workflow 'Verify protected release environment before privileged job admission'
require_workflow 'needs: [policy, candidate, release-policy-preflight]'
require_workflow 'environment: production-release'
require_workflow 'id-token: write'
require_workflow 'attestations: write'
require_workflow 'artifact-metadata: write'
require_workflow 'actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683'
require_workflow 'actions/upload-artifact@043fb46d1a93c77aae656e7c1c64a875d1fc6a0a'
require_workflow 'actions/download-artifact@70fc10c6e5e1ce46ad2ea6f2b72d43f7d47b13c3'
require_workflow 'actions/attest@1e69f48acb82d1966a394da916b4c1698aa569d6'
require_workflow 'check_protected_release_environment.sh'
require_workflow 'gh attestation verify'
require_workflow '--predicate-type https://spdx.dev/Document/v2.3'
require_workflow '--signer-workflow "$GITHUB_REPOSITORY/.github/workflows/release.yml"'
require_workflow '--source-digest "$GITHUB_SHA"'
require_workflow 'gh release create "$VERSION"'
require_workflow '--draft --verify-tag'
require_workflow 'gh release delete "$VERSION"'
require_workflow 'gh release edit "$VERSION"'
require_workflow 'DRY_RUN_ONLY: workflow_dispatch can build and verify candidates but cannot publish.'

require_version_policy 'refs/remotes/origin/master'
require_version_policy 'merge-base --is-ancestor'
require_version_policy 'release tag commit is not reachable from protected master lineage'

if grep -Eq '^[[:space:]]*pull_request:' "${WORKFLOW}"; then
  echo "error: privileged release workflow must not run on pull_request" >&2
  exit 1
fi
if grep -Fq 'continue-on-error:' "${WORKFLOW}"; then
  echo "error: release workflow cannot weaken mandatory publication checks with continue-on-error" >&2
  exit 1
fi
if grep -Eq 'uses:[[:space:]]+actions/(checkout|upload-artifact|download-artifact|attest)@v[0-9]' "${WORKFLOW}"; then
  echo "error: release workflow security-sensitive actions must be pinned to immutable commit SHAs" >&2
  exit 1
fi
if grep -Fq 'SHORTHAND_RELEASE_BASE_REF' "${WORKFLOW}"; then
  echo "error: production release workflow must not override the protected master lineage ref" >&2
  exit 1
fi

preflight_block="$(awk '/^  release-policy-preflight:/{capture=1} /^  publish:/{capture=0} capture' "${WORKFLOW}")"
[[ -n "${preflight_block}" ]] || { echo "error: release preflight block could not be isolated" >&2; exit 1; }
if grep -Eq '(contents|id-token|attestations|artifact-metadata):[[:space:]]*write' <<<"${preflight_block}"; then
  echo "error: release policy preflight must remain non-privileged" >&2
  exit 1
fi
if [[ "$(grep -Fc 'environment: production-release' "${WORKFLOW}")" != 1 ]]; then
  echo "error: production-release environment must be attached only to the privileged publish job" >&2
  exit 1
fi

bash tests/release/test_release_version_policy.sh
bash tests/release/test_release_environment_policy.sh
bash tests/release/test_release_bundle_tamper.sh

printf 'PASS signed release and protected publication contract gate\n'
