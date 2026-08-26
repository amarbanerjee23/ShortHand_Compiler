#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-}"
PREFIX="${2:-}"
CONSUMER_BUILD="${3:-${TMPDIR:-/tmp}/shorthand-installed-consumer-lifecycle}"
FIXTURE="${ROOT_DIR}/tests/packaging/installed_consumer"

if [[ -z "${BUILD_DIR}" || -z "${PREFIX}" ]]; then
  echo "usage: $0 <cmake-build-dir> <install-prefix> [consumer-build-dir]" >&2
  exit 2
fi

BUILD_DIR="$(cd "${BUILD_DIR}" && pwd)"
mkdir -p "${PREFIX}"
PREFIX="$(cd "${PREFIX}" && pwd)"
INSTALL_MANIFEST="${BUILD_DIR}/install_manifest.txt"
LIFECYCLE_ROOT="${CONSUMER_BUILD}-lifecycle"
FIRST_MANIFEST="${LIFECYCLE_ROOT}/first.sha256"
SECOND_MANIFEST="${LIFECYCLE_ROOT}/second.sha256"

rm -rf "${CONSUMER_BUILD}" "${LIFECYCLE_ROOT}"
mkdir -p "${LIFECYCLE_ROOT}"

normalize_manifest_path() {
  local path="$1"
  # Native Windows CMake writes install_manifest.txt with CRLF line endings.
  # Bash read -r preserves the trailing carriage return, so remove exactly that
  # record terminator without altering any valid path character.
  printf '%s' "${path%$'\r'}"
}

hash_install_manifest() {
  local output="$1"
  [[ -s "${INSTALL_MANIFEST}" ]] || { echo "error: CMake install manifest is missing" >&2; exit 1; }
  : >"${output}"
  while IFS= read -r installed; do
    installed="$(normalize_manifest_path "${installed}")"
    [[ -n "${installed}" ]] || continue
    # CMake writes native Windows paths (for example D:/...) into the install
    # manifest. Let CMake resolve those paths instead of asking an MSYS shell to
    # reinterpret them as POSIX paths.
    if ! cmake -E sha256sum "${installed}" >>"${output}"; then
      echo "error: installed SDK file missing or unreadable: ${installed}" >&2
      exit 1
    fi
  done < <(LC_ALL=C sort "${INSTALL_MANIFEST}")
}

cmake --install "${BUILD_DIR}"
hash_install_manifest "${FIRST_MANIFEST}"
bash "${ROOT_DIR}/scripts/check_installed_consumer_cmake.sh" "${PREFIX}" "${CONSUMER_BUILD}-first"

# Reinstalling the same release models an idempotent same-version upgrade. It
# must not mutate public headers, libraries, package metadata or executables.
cmake --install "${BUILD_DIR}"
hash_install_manifest "${SECOND_MANIFEST}"
cmp -s "${FIRST_MANIFEST}" "${SECOND_MANIFEST}" || {
  echo "error: reinstall changed installed SDK bytes" >&2
  diff -u "${FIRST_MANIFEST}" "${SECOND_MANIFEST}" >&2 || true
  exit 1
}
bash "${ROOT_DIR}/scripts/check_installed_consumer_cmake.sh" "${PREFIX}" "${CONSUMER_BUILD}-second"

# Uninstall exactly the files recorded by CMake. Use `cmake -E rm` so native
# Windows paths and POSIX paths follow the same code path.
while IFS= read -r installed; do
  installed="$(normalize_manifest_path "${installed}")"
  [[ -n "${installed}" ]] || continue
  cmake -E rm -f "${installed}"
done < "${INSTALL_MANIFEST}"

# Remove now-empty package directories without touching files outside PREFIX.
find "${PREFIX}" -depth -type d -empty -delete 2>/dev/null || true

if [[ -e "${PREFIX}/include/shorthand/runtime/ShorthandRuntime.h" || \
      -e "${PREFIX}/lib/cmake/ShortHand/ShortHandConfig.cmake" ]]; then
  echo "error: SDK uninstall left required package files behind" >&2
  exit 1
fi

set +e
cmake -S "${FIXTURE}" -B "${CONSUMER_BUILD}-after-uninstall" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${PREFIX}" \
  -DShortHand_DIR="${PREFIX}/lib/cmake/ShortHand" \
  -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE \
  -DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE \
  >"${LIFECYCLE_ROOT}/after-uninstall.out" 2>"${LIFECYCLE_ROOT}/after-uninstall.err"
status=$?
set -e
if [[ ${status} -eq 0 ]]; then
  echo "error: downstream consumer still configured successfully after SDK uninstall" >&2
  exit 1
fi
if ! grep -Eq 'ShortHand|find_package' "${LIFECYCLE_ROOT}/after-uninstall.out" "${LIFECYCLE_ROOT}/after-uninstall.err"; then
  cat "${LIFECYCLE_ROOT}/after-uninstall.out" >&2 || true
  cat "${LIFECYCLE_ROOT}/after-uninstall.err" >&2 || true
  echo "error: post-uninstall consumer failed for an unrelated reason" >&2
  exit 1
fi

echo "PASS installed ShortHand SDK install/reinstall/uninstall lifecycle"

# The existing mandatory Linux arm64 CI lane is also the native arm64 image
# qualification lane for roadmap PR77. This is deliberately fail-closed: when
# GitHub CI is running on the qualified aarch64 host, Docker availability and
# the production image build/runtime test are mandatory evidence, not a skip.
if [[ "${CI:-}" == "true" && "$(uname -s)" == "Linux" && "$(uname -m)" == "aarch64" ]]; then
  command -v docker >/dev/null 2>&1 || { echo "error: Docker is required for native Linux arm64 image qualification" >&2; exit 1; }
  docker info >/dev/null 2>&1 || { echo "error: Docker daemon unavailable for native Linux arm64 image qualification" >&2; exit 1; }
  docker build --pull --platform linux/arm64 -t shorthand:pr87-arm64 "${ROOT_DIR}"
  bash "${ROOT_DIR}/scripts/check_container_runtime.sh" shorthand:pr87-arm64 arm64
  echo "PASS native Linux arm64 production container qualification"
fi
