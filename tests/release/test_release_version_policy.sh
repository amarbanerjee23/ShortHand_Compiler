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

# Exercise the protected-master-lineage rule in an isolated repository so this
# test never creates or rewrites release tags in the checkout under test.
TMP_REPO="${TMPDIR:-/tmp}/shorthand-release-version-lineage"
rm -rf "${TMP_REPO}"
mkdir -p "${TMP_REPO}/scripts"
cp scripts/check_release_version_policy.sh "${TMP_REPO}/scripts/"
cat > "${TMP_REPO}/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.16)
project(ShortHand VERSION 1.0.0 LANGUAGES CXX)
EOF

git -C "${TMP_REPO}" init -q -b master
git -C "${TMP_REPO}" config user.name 'ShortHand CI'
git -C "${TMP_REPO}" config user.email 'ci@invalid.example'
git -C "${TMP_REPO}" add CMakeLists.txt scripts/check_release_version_policy.sh
git -C "${TMP_REPO}" commit -q -m baseline
master_sha="$(git -C "${TMP_REPO}" rev-parse HEAD)"
git -C "${TMP_REPO}" tag v1.0.0

env GITHUB_REF_TYPE=tag GITHUB_REF_NAME=v1.0.0 GITHUB_SHA="${master_sha}" \
  SHORTHAND_RELEASE_BASE_REF=refs/heads/master \
  bash "${TMP_REPO}/scripts/check_release_version_policy.sh" v1.0.0 --publish \
  >/tmp/shorthand_release_version_master_lineage.out

git -C "${TMP_REPO}" tag -d v1.0.0 >/dev/null
git -C "${TMP_REPO}" checkout -q --orphan untrusted-release
git -C "${TMP_REPO}" rm -q -rf .
mkdir -p "${TMP_REPO}/scripts"
cp scripts/check_release_version_policy.sh "${TMP_REPO}/scripts/"
cat > "${TMP_REPO}/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.16)
project(ShortHand VERSION 1.0.0 LANGUAGES CXX)
EOF
printf 'untrusted\n' > "${TMP_REPO}/side.txt"
git -C "${TMP_REPO}" add .
git -C "${TMP_REPO}" commit -q -m untrusted-release
side_sha="$(git -C "${TMP_REPO}" rev-parse HEAD)"
git -C "${TMP_REPO}" tag v1.0.0
expect_failure non-master-lineage env GITHUB_REF_TYPE=tag GITHUB_REF_NAME=v1.0.0 GITHUB_SHA="${side_sha}" \
  SHORTHAND_RELEASE_BASE_REF=refs/heads/master \
  bash "${TMP_REPO}/scripts/check_release_version_policy.sh" v1.0.0 --publish

printf 'PASS release version policy positive and negative matrix\n'
