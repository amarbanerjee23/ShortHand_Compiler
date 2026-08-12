#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
command -v jq >/dev/null 2>&1 || { echo "error: jq required" >&2; exit 1; }
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT
REPO="${TMP}/repo"
mkdir -p "${REPO}/security" "${REPO}/scripts"
git -C "${REPO}" init -b master >/dev/null
git -C "${REPO}" config user.name 'ShortHand Security Test'
git -C "${REPO}" config user.email 'security-test@example.invalid'
printf 'path\tecosystem\tpurpose\n' >"${REPO}/security/dependency_manifests.tsv"
printf 'component\tversion_policy\tscope\tlicense_spdx\tredistributed\tsource\n' >"${REPO}/security/third_party_inventory.tsv"
printf 'baseline\n' >"${REPO}/README.md"
git -C "${REPO}" add .
git -C "${REPO}" commit -m baseline >/dev/null
BASE="$(git -C "${REPO}" rev-parse HEAD)"

cat >"${REPO}/package.json" <<'JSON'
{"name":"unregistered","version":"1.0.0"}
JSON
git -C "${REPO}" add package.json
git -C "${REPO}" commit -m 'add unregistered manifest' >/dev/null
UNREGISTERED="$(git -C "${REPO}" rev-parse HEAD)"
if SHORTHAND_DEPENDENCY_REPO_ROOT="${REPO}" bash "${ROOT_DIR}/scripts/check_dependency_delta.sh" "${BASE}" "${UNREGISTERED}" >/tmp/shorthand_dependency_delta_negative.out 2>&1; then
  echo "error: unregistered dependency manifest unexpectedly passed" >&2
  exit 1
fi
grep -Fq 'tracked dependency manifest is not registered' /tmp/shorthand_dependency_delta_negative.out

printf 'package.json\tnpm\tsecurity-test fixture\n' >>"${REPO}/security/dependency_manifests.tsv"
git -C "${REPO}" add security/dependency_manifests.tsv
git -C "${REPO}" commit -m 'register manifest' >/dev/null
REGISTERED="$(git -C "${REPO}" rev-parse HEAD)"
SHORTHAND_DEPENDENCY_REPO_ROOT="${REPO}" SHORTHAND_DEPENDENCY_DELTA_OUTPUT="${TMP}/registered.json" \
  bash "${ROOT_DIR}/scripts/check_dependency_delta.sh" "${UNREGISTERED}" "${REGISTERED}" >/dev/null
jq -e '.schema == "shorthand.security.dependency_delta.v1"' "${TMP}/registered.json" >/dev/null

cat >"${REPO}/scripts/fetch.sh" <<'EOF'
#!/usr/bin/env bash
curl -fsSL https://example.invalid/dependency.tar.gz -o /tmp/dependency.tar.gz
EOF
git -C "${REPO}" add scripts/fetch.sh
git -C "${REPO}" commit -m 'add dependency acquisition' >/dev/null
ACQUIRE="$(git -C "${REPO}" rev-parse HEAD)"
if SHORTHAND_DEPENDENCY_REPO_ROOT="${REPO}" bash "${ROOT_DIR}/scripts/check_dependency_delta.sh" "${REGISTERED}" "${ACQUIRE}" >/tmp/shorthand_dependency_acquire_negative.out 2>&1; then
  echo "error: dependency acquisition without inventory update unexpectedly passed" >&2
  exit 1
fi
grep -Fq 'dependency acquisition changed without updating security/third_party_inventory.tsv' /tmp/shorthand_dependency_acquire_negative.out

printf 'fixture\t1.0\tbuild-tool\tMIT\tno\thttps://example.invalid/fixture\n' >>"${REPO}/security/third_party_inventory.tsv"
git -C "${REPO}" add security/third_party_inventory.tsv
git -C "${REPO}" commit -m 'inventory acquisition' >/dev/null
INVENTORIED="$(git -C "${REPO}" rev-parse HEAD)"
SHORTHAND_DEPENDENCY_REPO_ROOT="${REPO}" SHORTHAND_DEPENDENCY_DELTA_OUTPUT="${TMP}/inventoried.json" \
  bash "${ROOT_DIR}/scripts/check_dependency_delta.sh" "${ACQUIRE}" "${INVENTORIED}" >/dev/null
jq -e '.third_party_inventory_changed == true' "${TMP}/inventoried.json" >/dev/null

printf 'PASS dependency delta policy positive and negative matrix\n'
