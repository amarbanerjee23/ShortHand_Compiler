#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKFLOW="${ROOT_DIR}/.github/workflows/ci.yml"

if [[ ! -f "${WORKFLOW}" ]]; then
  echo "error: CI workflow is missing: ${WORKFLOW}" >&2
  exit 1
fi

require_text() {
  local haystack="$1"
  local needle="$2"
  local description="$3"
  if ! grep -Fq -- "${needle}" <<<"${haystack}"; then
    echo "error: ${description}: missing '${needle}'" >&2
    exit 1
  fi
}

workflow_text="$(cat "${WORKFLOW}")"
publish_block="$(awk '
  /- name: Publish event-specific CI status/ { capture=1 }
  capture { print }
' "${WORKFLOW}")"

require_text "${workflow_text}" 'group: ci-${{ github.workflow }}-${{ github.event_name }}-${{ github.ref }}' \
  "CI concurrency must remain isolated by event and ref"
require_text "${workflow_text}" 'cancel-in-progress: true' \
  "superseded CI runs must remain cancellable"
require_text "${publish_block}" 'if: ${{ always() && !cancelled() }}' \
  "cancelled runs must not publish a terminal commit status"
require_text "${publish_block}" 'TARGET_SHA: ${{ github.event.pull_request.head.sha || github.sha }}' \
  "event-specific status must target the intended source SHA"
require_text "${publish_block}" 'ci / ubuntu (%s)' \
  "required event-specific status context must remain stable"

if grep -Fq -- 'if: always()' <<<"${publish_block}"; then
  echo "error: status publisher must not run unconditionally after cancellation" >&2
  exit 1
fi

echo "PASS CI status hygiene guard"
