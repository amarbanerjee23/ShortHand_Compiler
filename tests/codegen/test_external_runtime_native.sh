#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/src"
BUILD_DIR="${ROOT_DIR}/Compiler_new_ws/Short_Hand/build"
WORK_DIR="/tmp/shorthand_external_runtime_native"
EXTERNAL_IR="${WORK_DIR}/IR_Generator.external.cpp"
EXTERNAL_BIN="${WORK_DIR}/short_hand_external"
FIXTURE="${ROOT_DIR}/tests/fixtures/external_runtime_ai.short"
RUNTIME_LIB="${SHORTHAND_RUNTIME_LIB:-${BUILD_DIR}/libshorthand_runtime.a}"

rm -rf "${WORK_DIR}"
mkdir -p "${WORK_DIR}"

make -C "${SRC_DIR}" parser.tab.cc lex.yy.c runtime_lib patch-generated-parser >/tmp/shorthand_external_runtime_make.out 2>&1
bash "${ROOT_DIR}/scripts/generate_external_runtime_ir_generator.sh" \
  "${SRC_DIR}/visitors/IR_Generator.cpp" \
  "${EXTERNAL_IR}"

if grep -Fq 'BasicBlock *entry = BasicBlock::Create(ShortGlobalContext, "entry", fn);' "${EXTERNAL_IR}"; then
  echo "FAIL external runtime generator left local hook stub body in generated IR_Generator" >&2
  exit 1
fi

grep -Fq 'return Function::Create(ftype, GlobalValue::ExternalLinkage, name, module);' "${EXTERNAL_IR}"
grep -Fq 'SHORTHAND_RUNTIME_LIB' "${EXTERNAL_IR}"
grep -Fq 'precision float;' "${FIXTURE}"

LLVM_CXXFLAGS="$(llvm-config --cxxflags)"
LLVM_LDFLAGS="$(llvm-config --ldflags --system-libs --libs core bitwriter)"
FLEX_LDLIB=""
if printf 'int main(){return 0;}\n' | c++ -x c++ - -lfl -o /tmp/shorthand_check_flex_lib >/dev/null 2>&1; then
  FLEX_LDLIB="-lfl"
fi
rm -f /tmp/shorthand_check_flex_lib

AI_RUNTIME_CORE_SRC="ai_runtime/AI_Runtime.cpp ai_runtime/AI_Types.cpp ai_runtime/AI_Telemetry.cpp ai_runtime/AI_Backend.cpp ai_runtime/backends/FallbackBackend.cpp ai_runtime/backends/OnnxRuntimeBackend.cpp ai_runtime/backends/TensorRTBackend.cpp ai_runtime/backends/OpenVINOBackend.cpp ai_runtime/backends/LibTorchBackend.cpp ai_runtime/backends/LlamaCppBackend.cpp"

(
  cd "${SRC_DIR}"
  c++ -O2 -Wall -Wextra -Wpedantic -std=c++17 -g \
    -Ivisitors \
    -DSHORTHAND_HAS_ONNXRUNTIME=0 \
    -DSHORTHAND_HAS_TENSORRT=0 \
    -DSHORTHAND_HAS_OPENVINO=0 \
    -DSHORTHAND_HAS_LIBTORCH=0 \
    -DSHORTHAND_HAS_LLAMACPP=0 \
    -DSHORTHAND_HAS_OPENBLAS=0 \
    -DSHORTHAND_HAS_EIGEN=0 \
    ${LLVM_CXXFLAGS} \
    -o "${EXTERNAL_BIN}" \
    main.cpp parser.tab.cc lex.yy.c ast/AST.cpp visitors/AST_Printer.cpp visitors/Interpreter.cpp "${EXTERNAL_IR}" visitors/SemanticAnalyzer.cpp visitors/Diagnostics.cpp evidence/EvidenceEmitter.cpp ${AI_RUNTIME_CORE_SRC} \
    ${FLEX_LDLIB} ${LLVM_LDFLAGS}
)

(
  cd "${WORK_DIR}"
  if ! SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" "${EXTERNAL_BIN}" "${FIXTURE}" compile >/tmp/shorthand_external_compile.out 2>&1; then
    echo "FAIL external runtime fixture did not compile" >&2
    cat /tmp/shorthand_external_compile.out >&2 || true
    exit 1
  fi
  grep -Fq 'declare i32 @short_ai_register_model' external_runtime_ai.ir
  grep -Fq 'declare i32 @short_ai_infer' external_runtime_ai.ir
  if grep -Fq 'define i32 @short_ai_register_model' external_runtime_ai.ir; then
    echo "FAIL generated IR still defines local short_ai_register_model stub" >&2
    exit 1
  fi

  if ! SHORTHAND_RUNTIME_LIB="${RUNTIME_LIB}" "${EXTERNAL_BIN}" "${FIXTURE}" compile-native >/tmp/shorthand_external_native.out 2>&1; then
    echo "FAIL external runtime fixture did not compile-native" >&2
    cat /tmp/shorthand_external_native.out >&2 || true
    exit 1
  fi
  ./external_runtime_ai >/tmp/shorthand_external_runtime_run.out 2>&1
)

grep -Fq 'Linked ShortHand runtime library' /tmp/shorthand_external_native.out
grep -Fq '[shorthand-runtime] model name=classifier' /tmp/shorthand_external_runtime_run.out
grep -Fq '[shorthand-runtime] infer model=classifier input=input output=output' /tmp/shorthand_external_runtime_run.out

echo "PASS external runtime native linking"
