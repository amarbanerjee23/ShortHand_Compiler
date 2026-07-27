#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
REPORT="/tmp/shorthand_backend_live_sdk_matrix.jsonl"
: > "${REPORT}"

record() {
  local backend="$1"
  local format="$2"
  local status="$3"
  local reason="$4"
  local gate="$5"
  printf 'BACKEND_MATRIX backend=%s format=%s status=%s reason=%s gate=%s\n' "${backend}" "${format}" "${status}" "${reason}" "${gate}"
  printf '{"schema":"shorthand.backend_live_sdk_matrix.v1","backend":"%s","format":"%s","status":"%s","reason":"%s","gate":"%s"}\n' \
    "${backend}" "${format}" "${status}" "${reason}" "${gate}" >> "${REPORT}"
}

run_onnxruntime_cpu() {
  if [[ -z "${ONNXRUNTIME_ROOT:-}" ]]; then
    record "onnxruntime_cpu" "onnx" "skip_safe" "ONNXRUNTIME_ROOT_not_set" "compiled_hook_onnxruntime_success"
    return 0
  fi

  if bash "${ROOT_DIR}/tests/integration/test_compiled_hook_onnxruntime_success.sh" \
      >/tmp/shorthand_backend_matrix_onnxruntime_cpu.out \
      2>/tmp/shorthand_backend_matrix_onnxruntime_cpu.err; then
    if grep -q 'PASS compiled hook ONNX Runtime success gate' /tmp/shorthand_backend_matrix_onnxruntime_cpu.out; then
      record "onnxruntime_cpu" "onnx" "live_success" "compiled_hook_success_fixture_passed" "compiled_hook_onnxruntime_success"
    else
      echo "error: ONNX Runtime gate completed without the expected PASS marker" >&2
      cat /tmp/shorthand_backend_matrix_onnxruntime_cpu.out >&2 || true
      cat /tmp/shorthand_backend_matrix_onnxruntime_cpu.err >&2 || true
      exit 1
    fi
  else
    echo "error: ONNX Runtime matrix row failed" >&2
    cat /tmp/shorthand_backend_matrix_onnxruntime_cpu.out >&2 || true
    cat /tmp/shorthand_backend_matrix_onnxruntime_cpu.err >&2 || true
    exit 1
  fi
}

run_tensorrt_rows() {
  if bash "${ROOT_DIR}/tests/integration/test_tensorrt_optional_fixture.sh" \
      >/tmp/shorthand_backend_matrix_tensorrt.out \
      2>/tmp/shorthand_backend_matrix_tensorrt.err; then
    if grep -q 'PASS tensorrt optional fixture gate' /tmp/shorthand_backend_matrix_tensorrt.out && \
       grep -q 'TENSORRT_FIXTURE backend=tensorrt status=unavailable_path_proved' /tmp/shorthand_backend_matrix_tensorrt.out; then
      record "onnxruntime_tensorrt" "onnx" "skip_safe" "onnxruntime_tensorrt_ep_fixture_not_enabled_no_false_success" "tensorrt_optional_fixture"
      record "tensorrt" "engine" "skip_safe" "tensorrt_unavailable_path_proved_no_false_success" "tensorrt_optional_fixture"
    else
      echo "error: TensorRT optional fixture completed without required proof markers" >&2
      cat /tmp/shorthand_backend_matrix_tensorrt.out >&2 || true
      cat /tmp/shorthand_backend_matrix_tensorrt.err >&2 || true
      exit 1
    fi
  else
    echo "error: TensorRT optional fixture gate failed" >&2
    cat /tmp/shorthand_backend_matrix_tensorrt.out >&2 || true
    cat /tmp/shorthand_backend_matrix_tensorrt.err >&2 || true
    exit 1
  fi
}

planned_backend() {
  local backend="$1"
  local format="$2"
  local env_var="$3"
  local planned_pr="$4"
  local value="${!env_var-}"

  if [[ -n "${value}" ]]; then
    record "${backend}" "${format}" "skip_safe" "${env_var}_set_but_dedicated_fixture_planned_${planned_pr}" "dedicated_fixture_planned"
  else
    record "${backend}" "${format}" "skip_safe" "${env_var}_not_set_policy_compatible_only" "policy_compatible_only"
  fi
}

require_matrix_row() {
  local backend="$1"
  if ! grep -q "\"backend\":\"${backend}\"" "${REPORT}"; then
    echo "error: backend matrix row missing for ${backend}" >&2
    cat "${REPORT}" >&2 || true
    exit 1
  fi
}

run_onnxruntime_cpu
run_tensorrt_rows
planned_backend "onnxruntime_cuda" "onnx" "ONNXRUNTIME_CUDA_ROOT" "PR57"
planned_backend "openvino" "openvino_ir" "OPENVINO_ROOT" "PR54"
planned_backend "libtorch" "torchscript" "LIBTORCH_ROOT" "PR55"
planned_backend "llamacpp" "gguf" "LLAMACPP_ROOT" "PR56"

require_matrix_row "onnxruntime_cpu"
require_matrix_row "onnxruntime_cuda"
require_matrix_row "onnxruntime_tensorrt"
require_matrix_row "tensorrt"
require_matrix_row "openvino"
require_matrix_row "libtorch"
require_matrix_row "llamacpp"

if grep -E '"backend":"(onnxruntime_cuda|onnxruntime_tensorrt|tensorrt|openvino|libtorch|llamacpp)","format":"[^"]+","status":"live_success"' "${REPORT}" >/dev/null; then
  echo "error: matrix harness must not claim live success for backends without dedicated live fixtures" >&2
  cat "${REPORT}" >&2 || true
  exit 1
fi

if ! grep -Eq '"status":"(live_success|skip_safe)"' "${REPORT}"; then
  echo "error: matrix harness produced no usable row status" >&2
  cat "${REPORT}" >&2 || true
  exit 1
fi

echo "Backend live SDK matrix report: ${REPORT}"
echo "PASS backend live SDK matrix harness"
