#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

required=(
  .github/workflows/ci.yml
  .github/workflows/security.yml
  .github/dependency-review-config.yml
  security/license_allowlist.txt
  security/third_party_inventory.tsv
  security/.trivyignore
  security/codeql_exceptions.tsv
  docs/external_security_policy.md
  scripts/check_third_party_license_policy.sh
  scripts/check_security_exceptions.sh
  scripts/check_action_pinning.sh
  scripts/check_codeql_sarif.sh
  scripts/check_trivy_report.sh
  scripts/assert_trivy_vulnerable_fixture.sh
  scripts/create_vulnerable_dependency_fixture.sh
  tests/security/test_security_policy_negative.sh
)
for file in "${required[@]}"; do [[ -s "${file}" ]] || { echo "error: missing PR77 security contract file: ${file}" >&2; exit 1; }; done
for script in scripts/check_third_party_license_policy.sh scripts/check_security_exceptions.sh scripts/check_action_pinning.sh scripts/check_codeql_sarif.sh scripts/check_trivy_report.sh scripts/assert_trivy_vulnerable_fixture.sh scripts/create_vulnerable_dependency_fixture.sh tests/security/test_security_policy_negative.sh; do bash -n "${script}"; done

bash scripts/check_third_party_license_policy.sh
bash scripts/check_security_exceptions.sh
bash scripts/check_action_pinning.sh
bash tests/security/test_security_policy_negative.sh

CI=.github/workflows/ci.yml
SECURITY_CI=.github/workflows/security.yml
require_ci() { grep -Fq -- "$1" "${CI}" || { echo "error: mandatory CI missing security contract: $1" >&2; exit 1; }; }
require_ci 'security:'
require_ci 'github/codeql-action/init@e4fba868fa4b1b91e1fdab776edc8cfbe6e9fb81'
require_ci 'github/codeql-action/analyze@e4fba868fa4b1b91e1fdab776edc8cfbe6e9fb81'
require_ci 'queries: security-extended'
require_ci 'upload: never'
require_ci 'aquasecurity/trivy-action@ed142fd0673e97e23eac54620cfb913e5ce36c25'
require_ci 'actions/dependency-review-action@a1d282b36b6f3519aa1f3fc636f609c47dddb294'
require_ci 'config-file: ./.github/dependency-review-config.yml'
require_ci 'tests/security/fixtures'
require_ci 'check_codeql_sarif.sh'
require_ci 'check_trivy_report.sh'
require_ci 'assert_trivy_vulnerable_fixture.sh'
require_ci 'needs.security.result'
require_ci '- security'

grep -Fq 'warn-only: false' .github/dependency-review-config.yml || { echo "error: dependency review cannot be warn-only" >&2; exit 1; }
if grep -Fq 'continue-on-error:' "${SECURITY_CI}"; then
  echo "error: scheduled security workflow cannot weaken findings with continue-on-error" >&2
  exit 1
fi
if awk '/^  security:/{inside=1} inside && /continue-on-error:/{bad=1} /^  [A-Za-z0-9_-]+:/ && !/^  security:/{if (inside) inside=0} END{exit bad?0:1}' "${CI}"; then
  echo "error: mandatory security job cannot use continue-on-error" >&2
  exit 1
fi

grep -Fq 'external_security_policy_version: shorthand.security.external.v1' docs/external_security_policy.md
grep -Fq 'tst018_status: implemented_after_required_scanners_pass' docs/external_security_policy.md
grep -Fq 'PASS Trivy vulnerable dependency negative fixture' scripts/assert_trivy_vulnerable_fixture.sh
grep -Fq 'PASS CodeQL C/C++ SAST policy gate' scripts/check_codeql_sarif.sh
grep -Fq 'PASS Trivy vulnerability and repository security policy gate' scripts/check_trivy_report.sh

printf 'PASS external vulnerability SAST dependency and license policy gate\n'
