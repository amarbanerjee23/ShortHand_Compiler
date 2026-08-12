#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="${TMPDIR:-/tmp}/shorthand-release-env-policy"
rm -rf "${TMP}"
mkdir -p "${TMP}"

cat > "${TMP}/protected.json" <<'JSON'
{"name":"production-release","protection_rules":[{"type":"required_reviewers","prevent_self_review":true,"reviewers":[{"type":"User","reviewer":{"login":"release-reviewer"}}]}],"deployment_branch_policy":{"protected_branches":false,"custom_branch_policies":true}}
JSON
cat > "${TMP}/policies.json" <<'JSON'
{"total_count":1,"branch_policies":[{"name":"v*","type":"branch"}]}
JSON
bash "${ROOT_DIR}/scripts/check_protected_release_environment.sh" "${TMP}/protected.json" "${TMP}/policies.json" >/tmp/shorthand_release_env_positive.out

expect_failure() {
  local name="$1"
  local env_file="$2"
  local policies_file="$3"
  if bash "${ROOT_DIR}/scripts/check_protected_release_environment.sh" "${env_file}" "${policies_file}" >/tmp/shorthand_release_env_negative.out 2>&1; then
    echo "error: release environment negative unexpectedly succeeded: ${name}" >&2
    exit 1
  fi
}

cat > "${TMP}/no-reviewers.json" <<'JSON'
{"name":"production-release","protection_rules":[],"deployment_branch_policy":{"protected_branches":false,"custom_branch_policies":true}}
JSON
expect_failure no-reviewers "${TMP}/no-reviewers.json" "${TMP}/policies.json"

cat > "${TMP}/self-review.json" <<'JSON'
{"name":"production-release","protection_rules":[{"type":"required_reviewers","prevent_self_review":false,"reviewers":[{"reviewer":{"login":"release-reviewer"}}]}],"deployment_branch_policy":{"protected_branches":false,"custom_branch_policies":true}}
JSON
expect_failure self-review "${TMP}/self-review.json" "${TMP}/policies.json"

cat > "${TMP}/no-custom-policy.json" <<'JSON'
{"name":"production-release","protection_rules":[{"type":"required_reviewers","prevent_self_review":true,"reviewers":[{"reviewer":{"login":"release-reviewer"}}]}],"deployment_branch_policy":{"protected_branches":true,"custom_branch_policies":false}}
JSON
expect_failure no-custom-policy "${TMP}/no-custom-policy.json" "${TMP}/policies.json"

cat > "${TMP}/wrong-policy.json" <<'JSON'
{"total_count":1,"branch_policies":[{"name":"main","type":"branch"}]}
JSON
expect_failure wrong-tag-policy "${TMP}/protected.json" "${TMP}/wrong-policy.json"

printf 'PASS protected release environment positive and negative policy matrix\n'
