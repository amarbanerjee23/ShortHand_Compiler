#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${SHORTHAND_ACTION_WORKFLOW_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.github/workflows" && pwd)}"
[[ -d "${ROOT_DIR}" ]] || { echo "error: workflow directory missing: ${ROOT_DIR}" >&2; exit 1; }
count=0
while IFS= read -r -d '' file; do
  while IFS= read -r line; do
    [[ "${line}" =~ uses:[[:space:]]*([^[:space:]#]+) ]] || continue
    ref="${BASH_REMATCH[1]}"
    count=$((count + 1))
    if [[ "${ref}" == ./* ]]; then
      continue
    fi
    if [[ "${ref}" == docker://*@sha256:* ]]; then
      continue
    fi
    [[ "${ref}" =~ ^[^@[:space:]]+@[0-9a-f]{40}$ ]] || {
      echo "error: external GitHub Action must use immutable 40-hex commit SHA: ${file}: ${ref}" >&2
      exit 1
    }
  done < "${file}"
done < <(find "${ROOT_DIR}" -type f \( -name '*.yml' -o -name '*.yaml' \) -print0 | LC_ALL=C sort -z)
[[ "${count}" -gt 0 ]] || { echo "error: no workflow action references were inspected" >&2; exit 1; }
printf 'PASS GitHub Actions immutable pinning gate refs=%d\n' "${count}"
