#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INVENTORY="${SHORTHAND_THIRD_PARTY_INVENTORY:-${ROOT_DIR}/security/third_party_inventory.tsv}"
ALLOWLIST="${SHORTHAND_LICENSE_ALLOWLIST:-${ROOT_DIR}/security/license_allowlist.txt}"

[[ -s "${INVENTORY}" ]] || { echo "error: missing third-party inventory: ${INVENTORY}" >&2; exit 1; }
[[ -s "${ALLOWLIST}" ]] || { echo "error: missing license allowlist: ${ALLOWLIST}" >&2; exit 1; }

expected=$'component\tversion_policy\tscope\tlicense_spdx\tredistributed\tsource'
[[ "$(head -n 1 "${INVENTORY}")" == "${expected}" ]] || { echo "error: third-party inventory header changed" >&2; exit 1; }

declare -A seen=()
count=0
while IFS=$'\t' read -r component version scope license redistributed source extra; do
  [[ -n "${component}" ]] || continue
  [[ -z "${extra:-}" ]] || { echo "error: malformed inventory row for ${component}" >&2; exit 1; }
  [[ -z "${seen[${component}]+x}" ]] || { echo "error: duplicate third-party component: ${component}" >&2; exit 1; }
  seen["${component}"]=1
  count=$((count + 1))

  case "${scope}" in
    build-tool|optional-sdk|linked-runtime|vendored) ;;
    *) echo "error: unsupported dependency scope ${scope} for ${component}" >&2; exit 1 ;;
  esac
  case "${redistributed}" in yes|no) ;; *) echo "error: redistributed must be yes/no for ${component}" >&2; exit 1 ;; esac
  [[ "${source}" =~ ^https:// ]] || { echo "error: dependency source must be HTTPS for ${component}" >&2; exit 1; }
  [[ -n "${license}" && "${license}" != "NOASSERTION" && "${license}" != "UNKNOWN" ]] || { echo "error: dependency license must be known for ${component}" >&2; exit 1; }
  [[ -n "${version}" ]] || { echo "error: dependency version policy missing for ${component}" >&2; exit 1; }
  lower="$(printf '%s' "${version}" | tr '[:upper:]' '[:lower:]')"
  if [[ "${lower}" == "latest" || "${lower}" == "unbounded" || "${version}" == *'*'* ]]; then
    echo "error: unbounded dependency version policy for ${component}: ${version}" >&2
    exit 1
  fi

  if [[ "${redistributed}" == "yes" || "${scope}" == "linked-runtime" || "${scope}" == "vendored" ]]; then
    [[ "${license}" != LicenseRef-* ]] || { echo "error: redistributed dependency cannot use LicenseRef: ${component}" >&2; exit 1; }
    grep -Fxq "${license}" "${ALLOWLIST}" || { echo "error: redistributed dependency license not allowlisted: ${component} ${license}" >&2; exit 1; }
  fi
done < <(tail -n +2 "${INVENTORY}")

[[ "${count}" -gt 0 ]] || { echo "error: third-party inventory is empty" >&2; exit 1; }
printf 'PASS third-party dependency and license policy gate components=%d\n' "${count}"
