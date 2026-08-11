#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREFIX="${1:-}"

if [[ -z "${PREFIX}" ]]; then
  echo "usage: $0 <installed-prefix> [consumer-build-dir]" >&2
  exit 2
fi
PREFIX="$(cd "${PREFIX}" && pwd)"
BUILD_DIR="${2:-${TMPDIR:-/tmp}/shorthand-installed-consumer-build}"
FIXTURE="${ROOT_DIR}/tests/packaging/installed_consumer"

for file in "${FIXTURE}/CMakeLists.txt" "${FIXTURE}/runtime_consumer.cpp" "${FIXTURE}/bridge_consumer.cpp"; do
  [[ -s "${file}" ]] || { echo "error: missing installed-consumer fixture: ${file}" >&2; exit 1; }
done

rm -rf "${BUILD_DIR}"
cmake -S "${FIXTURE}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${PREFIX}"
cmake --build "${BUILD_DIR}" --parallel 2

export PATH="${PREFIX}/bin${PATH:+:${PATH}}"
if [[ -d "${PREFIX}/lib" ]]; then
  export LD_LIBRARY_PATH="${PREFIX}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
  export DYLD_LIBRARY_PATH="${PREFIX}/lib${DYLD_LIBRARY_PATH:+:${DYLD_LIBRARY_PATH}}"
fi
if [[ -d "${PREFIX}/lib64" ]]; then
  export LD_LIBRARY_PATH="${PREFIX}/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
  export DYLD_LIBRARY_PATH="${PREFIX}/lib64${DYLD_LIBRARY_PATH:+:${DYLD_LIBRARY_PATH}}"
fi

ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "PASS installed ShortHand CMake consumer gate"
