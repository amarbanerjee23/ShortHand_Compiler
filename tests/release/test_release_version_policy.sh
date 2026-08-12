#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

bash scripts/check_release_version_policy.sh v1.0.0 >/tmp/shorthand_release_version_valid.out
bash scripts/check_release_version_policy.sh v1.0.0-rc.1 >/tmp/shorthand_release_version_rc.out

expect_failure() {
  local name="$1"
  shift
  if "$@" >/tmp/shorthand_release_version_negative.out 2>&1; then
    echo "error: release version negative unexpectedly succeeded: ${name}" >&2
    exit 1
  fi
}

expect_failure missing-v bash scripts/check_release_version_policy.sh 1.0.0
expect_failure incomplete bash scripts/check_release_version_policy.sh v1.0
expect_failure wrong-project-version bash scripts/check_release_version_policy.sh v2.0.0
expect_failure bad-rc bash scripts/check_release_version_policy.sh v1.0.0-rc.foo
expect_failure branch-publication env GITHUB_REF_TYPE=branch GITHUB_REF_NAME=v1.0.0 GITHUB_SHA="$(git rev-parse HEAD)" bash scripts/check_release_version_policy.sh v1.0.0 --publish

printf 'PASS release version policy positive and negative matrix\n'
