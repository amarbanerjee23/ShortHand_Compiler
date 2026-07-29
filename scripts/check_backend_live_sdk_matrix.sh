#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

require_file() {
  local file="$1"
  if [[ ! -f "${file}" ]]; then
    echo "error: missing required file: ${file}" >&2
    exit 1
  fi
}

require_contains() {
  local file="$1"
  local needle="$2"
  require_file "${file}"
  if ! grep -Fq "${needle}" "${file}"; then
    echo "error: ${file} missing required backend live SDK matrix text: ${needle}" >&2
    exit 1
  fi
}

require_file docs/backend_live_sdk_matrix.md
require_file docs/backend_compatibility_matrix.md
require_file docs/tensorrt_optional_fixture.md
require_file docs/openvino_optional_fixture.md
require_file docs/libtorch_optional_fixture.md
require_file docs/llamacpp_optional_fixture.md
require_file tests/integration/test_backend_live_sdk_matrix.sh
require_file tests/integration/test_compiled_hook_onnxruntime_success.sh
require_file tests/integration/test_tensorrt_optional_fixture.sh
require_file tests/integration/test_openvino_optional_fixture.sh
require_file tests/integration/test_libtorch_optional_fixture.sh
require_file tests/integration/test_llamacpp_optional_fixture.sh
require_file scripts/check_compiled_hook_onnxruntime_success.sh
require_file scripts/check_tensorrt_optional_fixture.sh
require_file scripts/check_openvino_optional_fixture.sh
require_file scripts/check_libtorch_optional_fixture.sh
require_file scripts/check_llamacpp_optional_fixture.sh

require_contains docs/backend_live_sdk_matrix.md 'backend_live_sdk_matrix_status: optional_matrix_harness'
require_contains docs/backend_live_sdk_matrix.md 'shorthand.backend_live_sdk_matrix.v1'
require_contains docs/backend_live_sdk_matrix.md 'live_success'
require_contains docs/backend_live_sdk_matrix.md 'skip_safe'
require_contains docs/backend_live_sdk_matrix.md 'policy_compatible_only'
require_contains docs/backend_live_sdk_matrix.md 'dedicated_fixture_planned'
require_contains docs/backend_live_sdk_matrix.md 'unavailable_path_proved'
require_contains docs/backend_live_sdk_matrix.md 'TensorRT unavailable-path proof'
require_contains docs/backend_live_sdk_matrix.md 'OpenVINO unavailable-path proof'
require_contains docs/backend_live_sdk_matrix.md 'LibTorch unavailable-path proof'
require_contains docs/backend_live_sdk_matrix.md 'Llama.cpp unavailable-path proof'
require_contains docs/backend_live_sdk_matrix.md 'Hardware-aware routing boundary'
require_contains docs/backend_live_sdk_matrix.md 'full_backend_matrix_claim: false'
require_contains docs/tensorrt_optional_fixture.md 'trt_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/openvino_optional_fixture.md 'openvino_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/libtorch_optional_fixture.md 'libtorch_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/llamacpp_optional_fixture.md 'llamacpp_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/llamacpp_optional_fixture.md 'production_claim_boundary: not production-executing yet'
require_contains docs/backend_compatibility_matrix.md 'Backend live SDK matrix harness'
require_contains docs/backend_compatibility_matrix.md 'backend_live_sdk_matrix_status: optional_matrix_harness'
require_contains docs/backend_compatibility_matrix.md 'llamacpp_optional_fixture_status: unavailable_path_proof_no_false_success'
require_contains docs/backend_compatibility_matrix.md 'Hardware capability discovery boundary'

require_contains tests/integration/test_backend_live_sdk_matrix.sh 'test_tensorrt_optional_fixture.sh'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'tensorrt_unavailable_path_proved_no_false_success'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'onnxruntime_tensorrt_ep_fixture_not_enabled_no_false_success'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'test_openvino_optional_fixture.sh'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'openvino_unavailable_path_proved_no_false_success'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'test_libtorch_optional_fixture.sh'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'libtorch_unavailable_path_proved_no_false_success'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'test_llamacpp_optional_fixture.sh'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'llamacpp_unavailable_path_proved_no_false_success'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'planned_backend "onnxruntime_cuda" "onnx" "ONNXRUNTIME_CUDA_ROOT" "PR58"'
require_contains tests/integration/test_backend_live_sdk_matrix.sh '"onnxruntime_cpu"'
require_contains tests/integration/test_backend_live_sdk_matrix.sh '"onnxruntime_cuda"'
require_contains tests/integration/test_backend_live_sdk_matrix.sh '"onnxruntime_tensorrt"'
require_contains tests/integration/test_backend_live_sdk_matrix.sh '"tensorrt"'
require_contains tests/integration/test_backend_live_sdk_matrix.sh '"openvino"'
require_contains tests/integration/test_backend_live_sdk_matrix.sh '"libtorch"'
require_contains tests/integration/test_backend_live_sdk_matrix.sh '"llamacpp"'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'compiled_hook_success_fixture_passed'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'dedicated_fixture_planned'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'policy_compatible_only'
require_contains tests/integration/test_backend_live_sdk_matrix.sh 'PASS backend live SDK matrix harness'

bash scripts/check_tensorrt_optional_fixture.sh
bash scripts/check_openvino_optional_fixture.sh
bash scripts/check_libtorch_optional_fixture.sh
bash scripts/check_llamacpp_optional_fixture.sh
bash tests/integration/test_backend_live_sdk_matrix.sh

echo "PASS backend live SDK matrix gate"
