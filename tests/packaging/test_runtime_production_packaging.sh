#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK_DIR="$(mktemp -d)"
BUILD_DIR="${WORK_DIR}/build"
INSTALL_DIR="${WORK_DIR}/install"
CONSUMER_DIR="${WORK_DIR}/consumer"
CONSUMER_BUILD_DIR="${WORK_DIR}/consumer-build"
trap 'rm -rf "${WORK_DIR}"' EXIT

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
  -DSHORTHAND_BUILD_TESTING=OFF
cmake --build "${BUILD_DIR}" --parallel 2 --target \
  shorthand_runtime shorthand_runtime_shared \
  shorthand_ai_bridge shorthand_ai_bridge_shared
cmake --install "${BUILD_DIR}"

require_installed() {
  local pattern="$1"
  if ! find "${INSTALL_DIR}" -path "${pattern}" -print -quit | grep -q .; then
    echo "error: missing installed path matching ${pattern}" >&2
    find "${INSTALL_DIR}" -maxdepth 5 -print >&2 || true
    exit 1
  fi
}

require_installed '*/include/shorthand/runtime/ShorthandRuntime.h'
require_installed '*/include/shorthand/runtime/AIRuntimeBridgeAdapter.h'
require_installed '*/include/shorthand/ai_runtime/AI_Types.h'
require_installed '*/include/shorthand/abi/shorthand_runtime_abi_v1.h'
require_installed '*/libshorthand_runtime.a'
require_installed '*/libshorthand_ai_bridge.a'
require_installed '*/ShortHandConfig.cmake'
require_installed '*/ShortHandConfigVersion.cmake'
require_installed '*/ShortHandTargets.cmake'
require_installed '*/shorthand-runtime.pc'
require_installed '*/shorthand-ai-bridge.pc'

RUNTIME_SHARED="$(find "${INSTALL_DIR}" -type f \( \
  -name 'libshorthand_runtime.so.1.0.0' -o \
  -name 'libshorthand_runtime.1.0.0.dylib' -o \
  -name 'shorthand_runtime.dll' \) -print -quit)"
BRIDGE_SHARED="$(find "${INSTALL_DIR}" -type f \( \
  -name 'libshorthand_ai_bridge.so.1.0.0' -o \
  -name 'libshorthand_ai_bridge.1.0.0.dylib' -o \
  -name 'shorthand_ai_bridge.dll' \) -print -quit)"

test -n "${RUNTIME_SHARED}"
test -n "${BRIDGE_SHARED}"

if command -v readelf >/dev/null 2>&1 && [[ "${RUNTIME_SHARED}" == *.so.* ]]; then
  readelf -d "${RUNTIME_SHARED}" | grep -Eq 'SONAME.*libshorthand_runtime\.so\.1'
  readelf -d "${BRIDGE_SHARED}" | grep -Eq 'SONAME.*libshorthand_ai_bridge\.so\.1'
fi

mkdir -p "${CONSUMER_DIR}"
cat > "${CONSUMER_DIR}/runtime_consumer.cpp" <<'CPP'
#include <runtime/ShorthandRuntime.h>

#include <cstring>

int main() {
    if (std::strcmp(short_runtime_abi_version(), "1.0.0") != 0) return 10;
    if (!short_runtime_is_abi_compatible(1, 0)) return 11;
    if (short_runtime_reset() != SHORTHAND_RUNTIME_OK) return 12;
    if (short_runtime_model_count() != 0) return 13;
    if (short_ai_register_tensor("input", "float", "1,4", "2", "4") != SHORTHAND_RUNTIME_OK) return 14;
    return short_runtime_tensor_count() == 1 ? 0 : 15;
}
CPP

cat > "${CONSUMER_DIR}/bridge_consumer.cpp" <<'CPP'
#include <runtime/AIRuntimeBridgeAdapter.h>

#include <cstring>

int main() {
    using namespace shorthand;
    if (std::strcmp(runtime_bridge::bridgeAdapterContractVersion(),
                    "shorthand.runtime.ai_runtime_execution_adapter.v1") != 0) return 20;
    if (runtime_bridge::runtimeStatusFromInferenceStatus(ai::InferenceStatus::Success) !=
        SHORTHAND_RUNTIME_OK) return 21;
    if (runtime_bridge::runtimeStatusFromInferenceStatus(ai::InferenceStatus::NotExecuted) !=
        SHORTHAND_RUNTIME_NOT_EXECUTED) return 22;
    return 0;
}
CPP

cat > "${CONSUMER_DIR}/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.16)
project(ShortHandInstalledConsumer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(ShortHand 1 CONFIG REQUIRED)

add_executable(runtime_static runtime_consumer.cpp)
target_link_libraries(runtime_static PRIVATE ShortHand::runtime)

add_executable(runtime_shared runtime_consumer.cpp)
target_link_libraries(runtime_shared PRIVATE ShortHand::runtime_shared)

add_executable(bridge_static bridge_consumer.cpp)
target_link_libraries(bridge_static PRIVATE ShortHand::ai_bridge)

add_executable(bridge_shared bridge_consumer.cpp)
target_link_libraries(bridge_shared PRIVATE ShortHand::ai_bridge_shared)
CMAKE

cmake -S "${CONSUMER_DIR}" -B "${CONSUMER_BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${INSTALL_DIR}"
cmake --build "${CONSUMER_BUILD_DIR}" --parallel 2

"${CONSUMER_BUILD_DIR}/runtime_static"
"${CONSUMER_BUILD_DIR}/runtime_shared"
"${CONSUMER_BUILD_DIR}/bridge_static"
"${CONSUMER_BUILD_DIR}/bridge_shared"

if command -v pkg-config >/dev/null 2>&1; then
  PKGCONFIG_DIR="$(find "${INSTALL_DIR}" -type d -name pkgconfig -print -quit)"
  LIB_DIR="$(dirname "${PKGCONFIG_DIR}")"
  export PKG_CONFIG_PATH="${PKGCONFIG_DIR}${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

  pkg-config --modversion shorthand-runtime | grep -Fx '1.0.0'
  pkg-config --modversion shorthand-ai-bridge | grep -Fx '1.0.0'

  ${CXX:-c++} -std=c++17 "${CONSUMER_DIR}/runtime_consumer.cpp" \
    $(pkg-config --cflags --libs shorthand-runtime) \
    -o "${WORK_DIR}/runtime-pkg-config"
  ${CXX:-c++} -std=c++17 "${CONSUMER_DIR}/bridge_consumer.cpp" \
    $(pkg-config --cflags --libs shorthand-ai-bridge) \
    -o "${WORK_DIR}/bridge-pkg-config"

  LD_LIBRARY_PATH="${LIB_DIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
    DYLD_LIBRARY_PATH="${LIB_DIR}${DYLD_LIBRARY_PATH:+:${DYLD_LIBRARY_PATH}}" \
    "${WORK_DIR}/runtime-pkg-config"
  LD_LIBRARY_PATH="${LIB_DIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
    DYLD_LIBRARY_PATH="${LIB_DIR}${DYLD_LIBRARY_PATH:+:${DYLD_LIBRARY_PATH}}" \
    "${WORK_DIR}/bridge-pkg-config"
fi

echo "PASS production runtime and AI bridge packaging consumer gate"
