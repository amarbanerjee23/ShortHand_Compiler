#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required stability text: ${needle}" >&2
    exit 1
  fi
}

require_bash_syntax() {
  local file="$1"
  require_file "${file}"
  bash -n "${file}"
}

require_file .github/workflows/ci.yml
require_file scripts/check_feature_plan_status.sh
require_file scripts/check_enterprise_hardening.sh
require_file docs/feature_implementation_status.md
require_file docs/pr_task_stability_strategy.md

# Preserve old task names and ordering anchors from the CI workflow.
require_contains .github/workflows/ci.yml 'Strict language validation'
require_contains .github/workflows/ci.yml 'Smoke tests'
require_contains .github/workflows/ci.yml 'Feature plan status check'
require_contains .github/workflows/ci.yml 'Enterprise hardening check'
require_contains .github/workflows/ci.yml 'Makefile test suite'
require_contains .github/workflows/ci.yml 'Certification bundle smoke'
require_contains .github/workflows/ci.yml 'Sanitizer tests'
require_contains .github/workflows/ci.yml 'Configure CMake'
require_contains .github/workflows/ci.yml 'Build with CMake'
require_contains .github/workflows/ci.yml 'Run CTest'

# Preserve feature tracker anchor phrases that old gates depend on.
require_contains docs/feature_implementation_status.md 'Automated SBOM'
require_contains docs/feature_implementation_status.md 'Runtime observability implementation'
require_contains docs/feature_implementation_status.md 'Module/import/package model'
require_contains docs/feature_implementation_status.md 'Production blockers'
require_contains docs/feature_implementation_status.md 'Compiled-code metadata/runtime lowering'

# Preserve existing enterprise hardening gate coverage before new gates are added.
require_contains scripts/check_enterprise_hardening.sh 'check_language_correctness.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_c3eco_claims_and_schema.sh'
require_contains scripts/check_enterprise_hardening.sh 'check_mlir_foundation.sh'

# Keep new guardrail scripts syntactically valid before they can block CI.
for script in scripts/check_feature_plan_status.sh scripts/check_enterprise_hardening.sh scripts/check_language_correctness.sh scripts/check_c3eco_claims_and_schema.sh scripts/check_mlir_foundation.sh; do
  require_bash_syntax "${script}"
done

# The strategy itself must explain the old-task contract so future changes do not silently rename CI anchors.
require_contains docs/pr_task_stability_strategy.md 'Old-task contract'
require_contains docs/pr_task_stability_strategy.md 'New-gate contract'

printf 'PASS PR task stability gate\n'
