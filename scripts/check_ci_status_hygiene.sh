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

extract_step() {
  local step_name="$1"
  awk -v target="- name: ${step_name}" '
    index($0, target) { capture=1 }
    capture && index($0, "- name: ") && !index($0, target) { exit }
    capture { print }
  ' "${WORKFLOW}"
}

workflow_text="$(cat "${WORKFLOW}")"
success_block="$(extract_step 'Publish stable event-specific CI status')"
failure_block="$(extract_step 'Publish stable event-specific CI failure')"

require_text "${workflow_text}" 'group: ci-${{ github.workflow }}-${{ github.event_name }}-${{ github.ref }}' \
  "CI concurrency must remain isolated by event and ref"
require_text "${workflow_text}" 'cancel-in-progress: true' \
  "superseded CI runs must remain cancellable"

require_text "${success_block}" 'if: ${{ always() && !cancelled() && success() }}' \
  "successful terminal status must not publish after cancellation"
require_text "${failure_block}" 'if: ${{ always() && !cancelled() && failure() }}' \
  "failed terminal status must not publish after cancellation"

for publish_block in "${success_block}" "${failure_block}"; do
  require_text "${publish_block}" 'HEAD_SHA: ${{ github.event.pull_request.head.sha || github.sha }}' \
    "event-specific status must target the intended source SHA"
  require_text "${publish_block}" 'context = f"ci / ubuntu ({event})"' \
    "required event-specific status context must remain stable"
done

if grep -Fq -- 'if: always()' <<<"${success_block}${failure_block}"; then
  echo "error: status publisher must not run unconditionally after cancellation" >&2
  exit 1
fi

echo "PASS CI status hygiene guard"
