#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OBJECTIVES="${ROOT_DIR}/docs/language_objectives.md"

require_contains() {
  local needle="$1"
  if ! grep -Fq "${needle}" "${OBJECTIVES}"; then
    echo "error: language objectives missing required text: ${needle}" >&2
    exit 1
  fi
}

if [[ ! -s "${OBJECTIVES}" ]]; then
  echo "error: missing language objectives document: ${OBJECTIVES}" >&2
  exit 1
fi

require_contains 'shorthand.language.objectives.version: 2026-07-29-v1'
require_contains 'objective_status: active_controlled_beta'
require_contains 'production_claim: false'
require_contains '## Mission'
require_contains '## Primary outcome'
require_contains '## Core language objectives'
require_contains 'Simple user-facing AI syntax'
require_contains 'Compiled C++/LLVM-first implementation'
require_contains 'Stable and versioned language contract'
require_contains 'Strict semantic correctness'
require_contains 'Honest execution and fallback'
require_contains 'Backend and hardware portability'
require_contains 'Deterministic operator control'
require_contains 'First-class observability'
require_contains 'First-class Green AI evidence'
require_contains 'Enterprise-scale program structure'
require_contains 'Secure and reproducible delivery'
require_contains 'Extensible compiler architecture'
require_contains 'Practical developer experience'
require_contains '## Priority order'
require_contains 'correctness and user safety'
require_contains 'Performance or energy improvements must not weaken quality'
require_contains '## Explicit non-goals'
require_contains 'silently execute through fallback while reporting success'
require_contains 'grant Green AI or C3-ECO certification'
require_contains '## Production success criteria'
require_contains '## Objective-to-roadmap alignment'
require_contains 'PR78 through PR79'
require_contains '## Change-control rule'

echo 'PASS language objectives gate'
