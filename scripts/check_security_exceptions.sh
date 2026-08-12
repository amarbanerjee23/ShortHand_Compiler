#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TRIVY_IGNORE="${SHORTHAND_TRIVY_IGNORE:-${ROOT_DIR}/security/.trivyignore}"
CODEQL_EXCEPTIONS="${SHORTHAND_CODEQL_EXCEPTIONS:-${ROOT_DIR}/security/codeql_exceptions.tsv}"
TODAY="${SECURITY_POLICY_TODAY:-$(date -u +%Y-%m-%d)}"
MAX_DAYS="${SECURITY_EXCEPTION_MAX_DAYS:-90}"

[[ "${TODAY}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || { echo "error: invalid policy date: ${TODAY}" >&2; exit 1; }
[[ "${MAX_DAYS}" =~ ^[0-9]+$ && "${MAX_DAYS}" -gt 0 ]] || { echo "error: invalid max exception days" >&2; exit 1; }
[[ -f "${TRIVY_IGNORE}" ]] || { echo "error: missing Trivy ignore file" >&2; exit 1; }
[[ -f "${CODEQL_EXCEPTIONS}" ]] || { echo "error: missing CodeQL exception file" >&2; exit 1; }

today_epoch="$(date -u -d "${TODAY}" +%s 2>/dev/null)" || { echo "error: policy date is not parseable" >&2; exit 1; }
max_epoch=$((today_epoch + MAX_DAYS * 86400))

validate_expiry() {
  local expires="$1" label="$2"
  [[ "${expires}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || { echo "error: ${label} has invalid expiry ${expires}" >&2; exit 1; }
  local epoch
  epoch="$(date -u -d "${expires}" +%s 2>/dev/null)" || { echo "error: ${label} expiry is not parseable" >&2; exit 1; }
  (( epoch > today_epoch )) || { echo "error: ${label} exception is expired or expires today: ${expires}" >&2; exit 1; }
  (( epoch <= max_epoch )) || { echo "error: ${label} exception exceeds ${MAX_DAYS}-day maximum: ${expires}" >&2; exit 1; }
}

trivy_count=0
while IFS= read -r raw || [[ -n "${raw}" ]]; do
  line="${raw#${raw%%[![:space:]]*}}"
  [[ -z "${line}" || "${line}" == \#* ]] && continue
  finding="${line%%[[:space:]]*}"
  [[ -n "${finding}" && "${finding}" != *'*'* && "${finding}" != *'?'* ]] || { echo "error: Trivy exception must not be wildcarded: ${raw}" >&2; exit 1; }
  [[ "${line}" =~ exp:([0-9]{4}-[0-9]{2}-[0-9]{2}) ]] || { echo "error: Trivy exception lacks exp:YYYY-MM-DD: ${raw}" >&2; exit 1; }
  expires="${BASH_REMATCH[1]}"
  validate_expiry "${expires}" "Trivy ${finding}"
  [[ "${line}" == *'owner='* && "${line}" == *'ticket='* && "${line}" == *'reason='* ]] || { echo "error: Trivy exception lacks owner/ticket/reason: ${raw}" >&2; exit 1; }
  trivy_count=$((trivy_count + 1))
done < "${TRIVY_IGNORE}"

expected=$'rule_id\tpath_prefix\texpires_on\towner\tticket\tjustification'
[[ "$(head -n 1 "${CODEQL_EXCEPTIONS}")" == "${expected}" ]] || { echo "error: CodeQL exception header changed" >&2; exit 1; }
codeql_count=0
while IFS=$'\t' read -r rule path expires owner ticket justification extra; do
  [[ -n "${rule}" ]] || continue
  [[ -z "${extra:-}" ]] || { echo "error: malformed CodeQL exception row" >&2; exit 1; }
  [[ "${rule}" != *'*'* && "${rule}" != *'?'* && -n "${path}" && "${path}" != *'*'* && "${path}" != *'?'* ]] || { echo "error: CodeQL exceptions cannot use wildcard rule/path" >&2; exit 1; }
  [[ -n "${owner}" && -n "${ticket}" && -n "${justification}" ]] || { echo "error: CodeQL exception lacks owner/ticket/justification" >&2; exit 1; }
  validate_expiry "${expires}" "CodeQL ${rule}:${path}"
  codeql_count=$((codeql_count + 1))
done < <(tail -n +2 "${CODEQL_EXCEPTIONS}")

printf 'PASS expiring security exception policy gate trivy=%d codeql=%d max_days=%d\n' "${trivy_count}" "${codeql_count}" "${MAX_DAYS}"
