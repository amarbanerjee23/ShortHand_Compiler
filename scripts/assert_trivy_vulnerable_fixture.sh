#!/usr/bin/env bash
set -euo pipefail
REPORT="${1:-}"
[[ -n "${REPORT}" && -f "${REPORT}" ]] || { echo "usage: $0 <trivy-vulnerable-fixture-report>" >&2; exit 2; }
command -v jq >/dev/null 2>&1 || { echo "error: jq is required" >&2; exit 1; }
jq -e '[.Results[]? | .Vulnerabilities[]? | select(.PkgName == "lodash" and ((.Severity // "") == "HIGH" or (.Severity // "") == "CRITICAL"))] | length > 0' "${REPORT}" >/dev/null || {
  echo "error: Trivy vulnerable fixture did not detect a HIGH/CRITICAL lodash vulnerability" >&2
  exit 1
}
printf 'PASS Trivy vulnerable dependency negative fixture\n'
