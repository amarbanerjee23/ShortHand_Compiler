#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${SHORTHAND_DEPENDENCY_REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
REGISTRY="${SHORTHAND_DEPENDENCY_MANIFEST_REGISTRY:-${ROOT_DIR}/security/dependency_manifests.tsv}"
BASE_REF="${1:-${SHORTHAND_DEPENDENCY_BASE_REF:-}}"
HEAD_REF="${2:-${SHORTHAND_DEPENDENCY_HEAD_REF:-HEAD}}"
OUTPUT="${SHORTHAND_DEPENDENCY_DELTA_OUTPUT:-/tmp/shorthand_dependency_delta.json}"

cd "${ROOT_DIR}"
command -v git >/dev/null 2>&1 || { echo "error: git is required for dependency delta review" >&2; exit 1; }
command -v jq >/dev/null 2>&1 || { echo "error: jq is required for dependency delta review" >&2; exit 1; }
[[ -s "${REGISTRY}" ]] || { echo "error: missing dependency manifest registry: ${REGISTRY}" >&2; exit 1; }
expected=$'path\tecosystem\tpurpose'
[[ "$(head -n 1 "${REGISTRY}")" == "${expected}" ]] || { echo "error: dependency manifest registry header changed" >&2; exit 1; }

if [[ -z "${BASE_REF}" || "${BASE_REF}" =~ ^0+$ ]]; then
  BASE_REF="$(git merge-base HEAD origin/master 2>/dev/null || git rev-parse HEAD^ 2>/dev/null || true)"
fi
[[ -n "${BASE_REF}" ]] || { echo "error: unable to determine dependency delta base ref" >&2; exit 1; }
git cat-file -e "${BASE_REF}^{commit}" 2>/dev/null || { echo "error: dependency delta base ref is unavailable: ${BASE_REF}" >&2; exit 1; }
git cat-file -e "${HEAD_REF}^{commit}" 2>/dev/null || { echo "error: dependency delta head ref is unavailable: ${HEAD_REF}" >&2; exit 1; }

is_manifest() {
  local path="$1" base="${1##*/}"
  case "${base}" in
    package.json|package-lock.json|npm-shrinkwrap.json|yarn.lock|pnpm-lock.yaml|Cargo.toml|Cargo.lock|go.mod|go.sum|vcpkg.json|vcpkg-configuration.json|conanfile.py|conanfile.txt|pyproject.toml|poetry.lock|Pipfile|Pipfile.lock|Gemfile|Gemfile.lock|composer.json|composer.lock) return 0 ;;
    requirements*.txt) return 0 ;;
  esac
  return 1
}

registry_paths="$(mktemp)"
tracked_manifests="$(mktemp)"
changed_manifests="$(mktemp)"
trap 'rm -f "${registry_paths}" "${tracked_manifests}" "${changed_manifests}"' EXIT

while IFS=$'\t' read -r path ecosystem purpose extra; do
  [[ -n "${path}" ]] || continue
  [[ -z "${extra:-}" ]] || { echo "error: malformed dependency manifest registry row: ${path}" >&2; exit 1; }
  [[ "${path}" != /* && "${path}" != *'..'* ]] || { echo "error: dependency manifest registry path must be repository relative: ${path}" >&2; exit 1; }
  case "${ecosystem}" in npm|cargo|go|vcpkg|conan|python|ruby|composer) ;; *) echo "error: unsupported dependency manifest ecosystem ${ecosystem} for ${path}" >&2; exit 1 ;; esac
  [[ -n "${purpose}" ]] || { echo "error: dependency manifest purpose missing for ${path}" >&2; exit 1; }
  git ls-files --error-unmatch -- "${path}" >/dev/null 2>&1 || { echo "error: registered dependency manifest is not tracked: ${path}" >&2; exit 1; }
  is_manifest "${path}" || { echo "error: registered path is not a recognized dependency manifest: ${path}" >&2; exit 1; }
  printf '%s\n' "${path}" >>"${registry_paths}"
done < <(tail -n +2 "${REGISTRY}")

if [[ -s "${registry_paths}" ]]; then
  duplicates="$(sort "${registry_paths}" | uniq -d)"
  [[ -z "${duplicates}" ]] || { echo "error: duplicate dependency manifest registry paths: ${duplicates}" >&2; exit 1; }
fi

while IFS= read -r path; do
  is_manifest "${path}" || continue
  printf '%s\n' "${path}" >>"${tracked_manifests}"
  grep -Fxq "${path}" "${registry_paths}" 2>/dev/null || { echo "error: tracked dependency manifest is not registered: ${path}" >&2; exit 1; }
done < <(git ls-files)

while IFS=$'\t' read -r status path rest; do
  [[ -n "${path}" ]] || continue
  if [[ "${status}" == R* || "${status}" == C* ]]; then
    path="${rest}"
  fi
  is_manifest "${path}" || continue
  printf '%s\n' "${path}" >>"${changed_manifests}"
  if [[ "${status}" != D* ]]; then
    grep -Fxq "${path}" "${registry_paths}" 2>/dev/null || { echo "error: changed dependency manifest is not registered: ${path}" >&2; exit 1; }
  fi
done < <(git diff --name-status "${BASE_REF}" "${HEAD_REF}")

inventory_changed=0
if ! git diff --quiet "${BASE_REF}" "${HEAD_REF}" -- security/third_party_inventory.tsv; then
  inventory_changed=1
fi

acquisition_lines="$(git diff --unified=0 "${BASE_REF}" "${HEAD_REF}" -- '*.cmake' 'CMakeLists.txt' 'scripts/*.sh' '.github/workflows/*.yml' '.github/workflows/*.yaml' \
  | awk '/^\+\+\+/ {next} /^\+/ {print substr($0,2)}' \
  | grep -E 'FetchContent_Declare|ExternalProject_Add|git[[:space:]]+clone|curl[^#]*https://|wget[^#]*https://' || true)"
if [[ -n "${acquisition_lines}" && "${inventory_changed}" -ne 1 ]]; then
  echo "error: dependency acquisition changed without updating security/third_party_inventory.tsv" >&2
  printf '%s\n' "${acquisition_lines}" >&2
  exit 1
fi

manifest_count="$(sed '/^[[:space:]]*$/d' "${changed_manifests}" | sort -u | wc -l | tr -d ' ')"
tracked_count="$(sed '/^[[:space:]]*$/d' "${tracked_manifests}" | sort -u | wc -l | tr -d ' ')"
acquisition_count=0
if [[ -n "${acquisition_lines}" ]]; then
  acquisition_count="$(printf '%s\n' "${acquisition_lines}" | wc -l | tr -d ' ')"
fi

mkdir -p "$(dirname "${OUTPUT}")"
jq -n \
  --arg schema 'shorthand.security.dependency_delta.v1' \
  --arg base "$(git rev-parse "${BASE_REF}^{commit}")" \
  --arg head "$(git rev-parse "${HEAD_REF}^{commit}")" \
  --argjson changed_manifests "${manifest_count}" \
  --argjson tracked_manifests "${tracked_count}" \
  --argjson inventory_changed "${inventory_changed}" \
  --argjson acquisition_lines "${acquisition_count}" \
  '{schema:$schema,base:$base,head:$head,changed_dependency_manifests:$changed_manifests,tracked_dependency_manifests:$tracked_manifests,third_party_inventory_changed:($inventory_changed==1),dependency_acquisition_additions:$acquisition_lines}' >"${OUTPUT}"

printf 'PASS dependency delta policy gate changed_manifests=%s tracked_manifests=%s inventory_changed=%s acquisition_additions=%s\n' \
  "${manifest_count}" "${tracked_count}" "${inventory_changed}" "${acquisition_count}"
