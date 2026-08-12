#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"
command -v jq >/dev/null 2>&1 || { echo "error: jq required for security policy negatives" >&2; exit 1; }
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

expect_failure() {
  local name="$1"; shift
  if "$@" >"${TMP}/${name}.out" 2>"${TMP}/${name}.err"; then
    echo "error: security negative unexpectedly succeeded: ${name}" >&2
    exit 1
  fi
}

cat >"${TMP}/bad_license.tsv" <<'EOF'
component	version_policy	scope	license_spdx	redistributed	source
badlib	1.0	vendored	GPL-3.0-only	yes	https://example.invalid/badlib
EOF
expect_failure prohibited-license env SHORTHAND_THIRD_PARTY_INVENTORY="${TMP}/bad_license.tsv" bash scripts/check_third_party_license_policy.sh

cat >"${TMP}/latest.tsv" <<'EOF'
component	version_policy	scope	license_spdx	redistributed	source
badlib	latest	optional-sdk	MIT	no	https://example.invalid/badlib
EOF
expect_failure unbounded-version env SHORTHAND_THIRD_PARTY_INVENTORY="${TMP}/latest.tsv" bash scripts/check_third_party_license_policy.sh

printf 'rule_id\tpath_prefix\texpires_on\towner\tticket\tjustification\n' >"${TMP}/codeql_empty.tsv"
cat >"${TMP}/expired.trivyignore" <<'EOF'
CVE-2020-0001 exp:2026-08-11 # owner=security ticket=SEC-1 reason=test
EOF
expect_failure expired-trivy env SECURITY_POLICY_TODAY=2026-08-12 SHORTHAND_TRIVY_IGNORE="${TMP}/expired.trivyignore" SHORTHAND_CODEQL_EXCEPTIONS="${TMP}/codeql_empty.tsv" bash scripts/check_security_exceptions.sh

cat >"${TMP}/bare.trivyignore" <<'EOF'
CVE-2020-0001 # owner=security ticket=SEC-1 reason=test
EOF
expect_failure no-expiry-trivy env SECURITY_POLICY_TODAY=2026-08-12 SHORTHAND_TRIVY_IGNORE="${TMP}/bare.trivyignore" SHORTHAND_CODEQL_EXCEPTIONS="${TMP}/codeql_empty.tsv" bash scripts/check_security_exceptions.sh

cat >"${TMP}/wild.trivyignore" <<'EOF'
CVE-* exp:2026-09-01 # owner=security ticket=SEC-1 reason=test
EOF
expect_failure wildcard-trivy env SECURITY_POLICY_TODAY=2026-08-12 SHORTHAND_TRIVY_IGNORE="${TMP}/wild.trivyignore" SHORTHAND_CODEQL_EXCEPTIONS="${TMP}/codeql_empty.tsv" bash scripts/check_security_exceptions.sh

: >"${TMP}/comments.trivyignore"
cat >"${TMP}/expired_codeql.tsv" <<'EOF'
rule_id	path_prefix	expires_on	owner	ticket	justification
cpp/test	src/	2026-08-11	security	SEC-2	test
EOF
expect_failure expired-codeql env SECURITY_POLICY_TODAY=2026-08-12 SHORTHAND_TRIVY_IGNORE="${TMP}/comments.trivyignore" SHORTHAND_CODEQL_EXCEPTIONS="${TMP}/expired_codeql.tsv" bash scripts/check_security_exceptions.sh

cat >"${TMP}/wild_codeql.tsv" <<'EOF'
rule_id	path_prefix	expires_on	owner	ticket	justification
cpp/*	src/*	2026-09-01	security	SEC-3	test
EOF
expect_failure wildcard-codeql env SECURITY_POLICY_TODAY=2026-08-12 SHORTHAND_TRIVY_IGNORE="${TMP}/comments.trivyignore" SHORTHAND_CODEQL_EXCEPTIONS="${TMP}/wild_codeql.tsv" bash scripts/check_security_exceptions.sh

mkdir -p "${TMP}/workflow"
cat >"${TMP}/workflow/pinned.yml" <<'EOF'
name: pinned
permissions:
  statuses: write
jobs:
  good:
    steps:
      - uses: actions/checkout@d23441a48e516b6c34aea4fa41551a30e30af803
EOF
env SHORTHAND_ACTION_WORKFLOW_ROOT="${TMP}/workflow" bash scripts/check_action_pinning.sh >/dev/null
cat >"${TMP}/workflow/floating.yml" <<'EOF'
name: bad
jobs:
  bad:
    steps:
      - uses: actions/checkout@v6
EOF
expect_failure floating-action env SHORTHAND_ACTION_WORKFLOW_ROOT="${TMP}/workflow" bash scripts/check_action_pinning.sh

cat >"${TMP}/high.sarif" <<'EOF'
{"version":"2.1.0","runs":[{"tool":{"driver":{"name":"CodeQL","rules":[{"id":"cpp/high-test","properties":{"security-severity":"9.8"}}]}},"results":[{"ruleId":"cpp/high-test","locations":[{"physicalLocation":{"artifactLocation":{"uri":"src/high.cpp"}}}]}]}]}
EOF
expect_failure high-codeql bash scripts/check_codeql_sarif.sh "${TMP}/high.sarif"
cat >"${TMP}/low.sarif" <<'EOF'
{"version":"2.1.0","runs":[{"tool":{"driver":{"name":"CodeQL","rules":[{"id":"cpp/low-test","properties":{"security-severity":"3.1"}}]}},"results":[{"ruleId":"cpp/low-test","locations":[{"physicalLocation":{"artifactLocation":{"uri":"src/low.cpp"}}}]}]}]}
EOF
bash scripts/check_codeql_sarif.sh "${TMP}/low.sarif" >/dev/null

cat >"${TMP}/trivy_high.json" <<'EOF'
{"Results":[{"Target":"fixture","Vulnerabilities":[{"VulnerabilityID":"CVE-TEST-1","PkgName":"badlib","Severity":"HIGH"}]}]}
EOF
expect_failure high-trivy bash scripts/check_trivy_report.sh "${TMP}/trivy_high.json"
cat >"${TMP}/trivy_secret.json" <<'EOF'
{"Results":[{"Target":"fixture","Secrets":[{"RuleID":"secret-test","Severity":"HIGH"}]}]}
EOF
expect_failure secret-trivy bash scripts/check_trivy_report.sh "${TMP}/trivy_secret.json"
printf '%s\n' '{"Results":[]}' >"${TMP}/trivy_clean.json"
bash scripts/check_trivy_report.sh "${TMP}/trivy_clean.json" >/dev/null

printf 'PASS external security policy negative matrix\n'
