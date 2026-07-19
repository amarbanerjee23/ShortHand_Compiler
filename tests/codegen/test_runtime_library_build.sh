#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
RUNTIME_LIB="${SHORTHAND_RUNTIME_LIB:-${ROOT_DIR}/Compiler_new_ws/Short_Hand/build/libshorthand_runtime.a}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

if [[ ! -f "${RUNTIME_LIB}" ]]; then
  make -C "${SRC_DIR}" runtime_lib >/tmp/shorthand_runtime_lib_build.out 2>&1
fi

test -s "${RUNTIME_LIB}"

cat > "${WORK_DIR}/runtime_probe.cpp" <<'CPP'
#include "runtime/ShorthandRuntime.h"
int main() {
    short_ai_register_tensor("input", "float", "1,4", "2", "4");
    short_ai_infer("classifier", "input", "output");
    short_greenai_record_measurement("workload", "classifier", "1", "1", "1");
    return 0;
}
CPP

${CXX:-g++} -std=c++17 -I"${SRC_DIR}" "${WORK_DIR}/runtime_probe.cpp" "${RUNTIME_LIB}" -o "${WORK_DIR}/runtime_probe"
"${WORK_DIR}/runtime_probe" >/tmp/shorthand_runtime_probe.out 2>/tmp/shorthand_runtime_probe.err

grep -q '\[shorthand-runtime\] tensor' /tmp/shorthand_runtime_probe.err
grep -q '\[shorthand-runtime\] infer' /tmp/shorthand_runtime_probe.err
grep -q '\[shorthand-runtime\] greenai_measure' /tmp/shorthand_runtime_probe.err

echo "PASS shorthand runtime library builds and resolves exported hooks"
