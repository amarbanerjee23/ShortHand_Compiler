#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="${ROOT_DIR}/scripts/check_fuzz_sanitizers.sh"

usage() {
  cat >&2 <<'EOF'
usage: replay_fuzz_reproducer.sh <parser|module|semantic|lowering> <artifact> [--minimize]

Rebuilds the PR73 instrumented libFuzzer targets with the same ASan/LSan/UBSan
policy and replays one saved crash artifact. With --minimize, libFuzzer runs
its crash minimizer and writes minimized artifacts beside the replay outputs.
EOF
}

[[ "$#" -eq 2 || "$#" -eq 3 ]] || { usage; exit 2; }
TARGET="$1"
INPUT="$2"
MODE="${3:-}"

case "${TARGET}" in
  parser|module|semantic|lowering) ;;
  *) echo "error: invalid fuzz target: ${TARGET}" >&2; usage; exit 2 ;;
esac
[[ -f "${INPUT}" ]] || { echo "error: fuzz artifact not found: ${INPUT}" >&2; exit 2; }
[[ -z "${MODE}" || "${MODE}" == "--minimize" ]] || { usage; exit 2; }

INPUT="$(cd "$(dirname "${INPUT}")" && pwd)/$(basename "${INPUT}")"
ARTIFACT_DIR="${SHORTHAND_FUZZ_ARTIFACT_DIR:-/tmp/shorthand_fuzz_replay_artifacts}"
mkdir -p "${ARTIFACT_DIR}"
LOG="/tmp/shorthand_fuzz_reproducer_${TARGET}.out"

set +e
SHORTHAND_FUZZ_REPLAY_TARGET="${TARGET}" \
SHORTHAND_FUZZ_REPLAY_INPUT="${INPUT}" \
SHORTHAND_FUZZ_MINIMIZE_CRASH="$([[ "${MODE}" == "--minimize" ]] && printf 1 || printf 0)" \
SHORTHAND_FUZZ_ARTIFACT_DIR="${ARTIFACT_DIR}" \
  bash "${CHECKER}" >"${LOG}" 2>&1
status=$?
set -e

cat "${LOG}"
grep -Fq "REPLAY_EXECUTED target=${TARGET}" "${LOG}" || {
  echo "error: fuzz target did not reach reproducer execution; this is a build/harness failure, not a reproduced crash" >&2
  exit 1
}

if [[ "${MODE}" == "--minimize" ]]; then
  if (( status != 0 )); then
    echo "error: libFuzzer crash minimization failed with status ${status}" >&2
    exit "${status}"
  fi
  printf 'PASS fuzz reproducer minimization target=%s artifact_dir=%s\n' "${TARGET}" "${ARTIFACT_DIR}"
  exit 0
fi

if (( status == 0 )); then
  echo "error: saved fuzz artifact no longer reproduces under the PR73 sanitizer policy" >&2
  exit 1
fi

printf 'PASS fuzz crash reproduced target=%s status=%s input=%s\n' "${TARGET}" "${status}" "${INPUT}"
