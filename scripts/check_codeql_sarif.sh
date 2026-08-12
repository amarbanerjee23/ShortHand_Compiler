#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT="${1:-}"
THRESHOLD="${SHORTHAND_CODEQL_SECURITY_SEVERITY_THRESHOLD:-7.0}"
EXCEPTIONS="${SHORTHAND_CODEQL_EXCEPTIONS:-${ROOT_DIR}/security/codeql_exceptions.tsv}"
[[ -n "${INPUT}" ]] || { echo "usage: $0 <sarif-file-or-directory>" >&2; exit 2; }
command -v jq >/dev/null 2>&1 || { echo "error: jq is required for CodeQL SARIF enforcement" >&2; exit 1; }
[[ "${THRESHOLD}" =~ ^[0-9]+([.][0-9]+)?$ ]] || { echo "error: invalid CodeQL security-severity threshold" >&2; exit 1; }
[[ -f "${EXCEPTIONS}" ]] || { echo "error: missing CodeQL exception policy" >&2; exit 1; }

bash "${ROOT_DIR}/scripts/check_security_exceptions.sh" >/tmp/shorthand_security_exceptions_codeql.out

files=()
if [[ -d "${INPUT}" ]]; then
  while IFS= read -r -d '' file; do files+=("${file}"); done < <(find "${INPUT}" -type f \( -name '*.sarif' -o -name '*.sarif.json' \) -print0 | LC_ALL=C sort -z)
elif [[ -f "${INPUT}" ]]; then
  files+=("${INPUT}")
else
  echo "error: CodeQL SARIF input does not exist: ${INPUT}" >&2
  exit 1
fi
[[ "${#files[@]}" -gt 0 ]] || { echo "error: no CodeQL SARIF files found" >&2; exit 1; }

high=0
excepted=0
results=0
for file in "${files[@]}"; do
  jq -e 'type == "object" and (.runs | type == "array") and (.runs | length > 0)' "${file}" >/dev/null || { echo "error: malformed SARIF: ${file}" >&2; exit 1; }
  while IFS=$'\t' read -r rule severity uri; do
    [[ -n "${rule}" ]] || continue
    results=$((results + 1))
    if awk -v sev="${severity}" -v threshold="${THRESHOLD}" 'BEGIN { exit !(sev + 0 >= threshold + 0) }'; then
      matched=0
      while IFS=$'\t' read -r erule epath expires owner ticket justification extra; do
        [[ -n "${erule}" ]] || continue
        if [[ "${erule}" == "${rule}" && "${uri}" == "${epath}"* ]]; then
          matched=1
          break
        fi
      done < <(tail -n +2 "${EXCEPTIONS}")
      if [[ "${matched}" -eq 1 ]]; then
        excepted=$((excepted + 1))
      else
        high=$((high + 1))
        printf 'error: CodeQL high-severity finding rule=%s security_severity=%s path=%s\n' "${rule}" "${severity}" "${uri}" >&2
      fi
    fi
  done < <(jq -r '
    .runs[] as $run
    | ($run.tool.driver.rules // []) as $rules
    | ($run.results // [])[]
    | . as $result
    | ($result.ruleId // "") as $id
    | ([ $rules[]? | select(.id == $id) | (.properties["security-severity"] // "0") ][0] // "0") as $severity
    | [$id, ($severity|tostring), ($result.locations[0].physicalLocation.artifactLocation.uri // "")]
    | @tsv' "${file}")
done

[[ "${high}" -eq 0 ]] || { echo "error: CodeQL SAST policy rejected ${high} high-severity finding(s)" >&2; exit 1; }
printf 'PASS CodeQL C/C++ SAST policy gate sarif=%d results=%d excepted=%d threshold=%s\n' "${#files[@]}" "${results}" "${excepted}" "${THRESHOLD}"
