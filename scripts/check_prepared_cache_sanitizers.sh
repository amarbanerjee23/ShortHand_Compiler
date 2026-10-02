#!/usr/bin/env bash
# Instrument ShortHand's real ONNX bridge and workspace, not just the test driver.
set -Eeuo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDK="$(cd "${1:?ONNX Runtime SDK required}" && pwd)"
BUILD_DIR="${2:?separate sanitizer build directory required}"
CXX_BIN="${CXX:-clang++-18}"
SAN_FLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer'
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:strict_string_checks=1
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
export ORT_DISABLE_TELEMETRY=1
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER="$CXX_BIN" -DCMAKE_CXX_FLAGS="$SAN_FLAGS" \
  -DSHORTHAND_BUILD_TESTING=OFF -DSHORTHAND_BUILD_MLIR=OFF \
  -DSHORTHAND_ENABLE_ONNXRUNTIME=ON -DSHORTHAND_STRICT_OPTIONAL_BACKENDS=ON \
  -DONNXRUNTIME_ROOT="$SDK"
cmake --build "$BUILD_DIR" --parallel 1 --target shorthand_runtime
SRC="$ROOT_DIR/Compiler_new_ws/Short_Hand/src/ai_runtime"
"$CXX_BIN" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
  -fno-omit-frame-pointer -ffunction-sections -fdata-sections -pthread -Wall -Wextra -Wpedantic -Werror \
  -I"$ROOT_DIR" "$ROOT_DIR/tests/ai_application/test_classification_host.cpp" \
  "$SRC/ApplicationQualification.cpp" "$SRC/AI_Types.cpp" -Wl,--gc-sections \
  -o "$BUILD_DIR/test-workspace-sanitized"
run_sanitized() {
  local label="$1"; shift
  local log="$BUILD_DIR/${label}-sanitizer.log"
  set +e
  ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:strict_string_checks=1 \
    UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 "$@" >"$log" 2>&1
  local status=$?
  set -e
  if (( status == 0 )); then
    cat "$log"
    return 0
  fi
  if grep -Eq 'LeakSanitizer has encountered a fatal error|LeakSanitizer.*does not work under ptrace' "$log"; then
    echo "WARN $label: LSan unavailable on this runner; rerunning ASan/UBSan with leak detection disabled" >&2
    ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:strict_string_checks=1 \
      UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 "$@"
    echo "LSan unavailable: runner rejected /proc inspection" >&2
    return 0
  fi
  cat "$log" >&2
  return "$status"
}

run_sanitized prepared-cache env CXX="$CXX_BIN" python3 "$ROOT_DIR/tests/ai_application/test_compiled_cache.py" "$BUILD_DIR" "$SDK" --sanitizers
run_sanitized workspace "$BUILD_DIR/test-workspace-sanitized"
echo 'PASS prepared cache and workspace ASan/UBSan (LSan enabled when runner permits it)'
# ORT is the pinned vendor binary and is not itself sanitizer-instrumented.
