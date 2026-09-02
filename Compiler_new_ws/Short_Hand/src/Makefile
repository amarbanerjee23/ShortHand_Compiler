# Compatibility build harness for CI and sanitizer gates.
# CMake is the production build system, but this Makefile remains intentionally
# maintained until every Makefile-backed CI target has an equivalent CTest gate.
CXX ?= g++
LLVM_CONFIG ?= llvm-config
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags 2>/dev/null)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --system-libs --libs core bitwriter 2>/dev/null)
CXXFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c++17 -g
LDFLAGS ?=
FLEX_LDLIB := $(shell printf 'int main(){return 0;}\n' | $(CXX) -x c++ - -lfl -o /tmp/shorthand_check_flex_lib >/dev/null 2>&1 && echo -lfl; rm -f /tmp/shorthand_check_flex_lib)
LDLIBS ?= $(FLEX_LDLIB)
ONNXRUNTIME_ROOT ?=
LIBTORCH_ROOT ?=
TENSORRT_ROOT ?=
CUDA_ROOT ?=
OPENVINO_ROOT ?=
LLAMACPP_ROOT ?=
EIGEN_ROOT ?=
OPENBLAS_ROOT ?=

SHORTHAND_HAS_ONNXRUNTIME := 0
SHORTHAND_HAS_TENSORRT := 0
SHORTHAND_HAS_OPENVINO := 0
SHORTHAND_HAS_LIBTORCH := 0
SHORTHAND_HAS_LLAMACPP := 0
SHORTHAND_HAS_OPENBLAS := 0
SHORTHAND_HAS_EIGEN := 0
AI_LDLIBS :=

ifneq ($(ONNXRUNTIME_ROOT),)
SHORTHAND_HAS_ONNXRUNTIME := 1
AI_LDLIBS += -L$(ONNXRUNTIME_ROOT)/lib -lonnxruntime
endif
ifneq ($(TENSORRT_ROOT),)
SHORTHAND_HAS_TENSORRT := 1
endif
ifneq ($(OPENVINO_ROOT),)
SHORTHAND_HAS_OPENVINO := 1
endif
ifneq ($(LIBTORCH_ROOT),)
SHORTHAND_HAS_LIBTORCH := 1
AI_LDLIBS += -L$(LIBTORCH_ROOT)/lib -ltorch -ltorch_cpu -lc10
endif
ifneq ($(LLAMACPP_ROOT),)
SHORTHAND_HAS_LLAMACPP := 1
endif
ifneq ($(OPENBLAS_ROOT),)
SHORTHAND_HAS_OPENBLAS := 1
endif
ifneq ($(EIGEN_ROOT),)
SHORTHAND_HAS_EIGEN := 1
endif

OPTIONAL_BACKEND_DEFS := -DSHORTHAND_HAS_ONNXRUNTIME=$(SHORTHAND_HAS_ONNXRUNTIME) -DSHORTHAND_HAS_TENSORRT=$(SHORTHAND_HAS_TENSORRT) -DSHORTHAND_HAS_OPENVINO=$(SHORTHAND_HAS_OPENVINO) -DSHORTHAND_HAS_LIBTORCH=$(SHORTHAND_HAS_LIBTORCH) -DSHORTHAND_HAS_LLAMACPP=$(SHORTHAND_HAS_LLAMACPP) -DSHORTHAND_HAS_OPENBLAS=$(SHORTHAND_HAS_OPENBLAS) -DSHORTHAND_HAS_EIGEN=$(SHORTHAND_HAS_EIGEN)
AI_CXXFLAGS := $(OPTIONAL_BACKEND_DEFS)
ONNX_EXCEPTION_CXXFLAGS :=
ifneq ($(ONNXRUNTIME_ROOT),)
AI_CXXFLAGS += -I$(ONNXRUNTIME_ROOT)/include
# llvm-config may inject -fno-exceptions; ONNX Runtime C++ uses Ort::Exception.
# Keep recoverable backend failures by making -fexceptions the final policy flag.
ONNX_EXCEPTION_CXXFLAGS := -fexceptions
endif
ifneq ($(LIBTORCH_ROOT),)
AI_CXXFLAGS += -I$(LIBTORCH_ROOT)/include -I$(LIBTORCH_ROOT)/include/torch/csrc/api/include
endif

AI_RUNTIME_CORE_SRC := ai_runtime/AI_Runtime.cpp ai_runtime/AI_Types.cpp ai_runtime/AI_Telemetry.cpp ai_runtime/AI_Backend.cpp ai_runtime/backends/FallbackBackend.cpp ai_runtime/backends/OnnxRuntimeBackend.cpp ai_runtime/backends/TensorRTBackend.cpp ai_runtime/backends/OpenVINOBackend.cpp ai_runtime/backends/LibTorchBackend.cpp ai_runtime/backends/LlamaCppBackend.cpp
SRC := main.cpp parser.tab.cc lex.yy.c ast/AST.cpp module/ModuleResolver.cpp module/Sha256.cpp type_system/ProductionTypeSystem.cpp enterprise/EnterpriseLanguage.cpp visitors/AST_Printer.cpp visitors/Interpreter.cpp visitors/IR_Generator.cpp visitors/IR_Generator_Module.cpp visitors/SemanticAnalyzer.cpp visitors/Diagnostics.cpp evidence/EvidenceEmitter.cpp $(AI_RUNTIME_CORE_SRC)
BIN := ../build/short_hand
AI_RUNTIME_SRC := $(AI_RUNTIME_CORE_SRC)
AI_TRAIN_SRC := ai_runtime/TorchTrainer.cpp
AI_APP_BIN := ../build/short_ai_app
AI_TRAIN_BIN := ../build/short_ai_train
GREEN_AI_TOOL_BIN := ../build/green_ai_tool
GREEN_AI_TOOL_SRC := green_ai/GreenAITool.cpp
C3ECO_MEASURE_BIN := ../build/shorthand_c3eco_measure
C3ECO_MEASURE_SRC := evidence/MeasurementWorkbook.cpp
SHORTHAND_RUNTIME_SRC := runtime/ShorthandRuntime.cpp
SHORTHAND_RUNTIME_FACADE_SRC := runtime/RuntimeThreadSafeFacade.cpp
SHORTHAND_RUNTIME_OBJ := ../build/ShorthandRuntimeImpl.o
SHORTHAND_RUNTIME_FACADE_OBJ := ../build/RuntimeThreadSafeFacade.o
SHORTHAND_RUNTIME_LIB := ../build/libshorthand_runtime.a
SHORTHAND_SERVING_SRC := serving/ServingRuntime.cpp
SHORTHAND_SERVING_WORKER_SRC := serving/ServingWorkerMain.cpp
SHORTHAND_SERVING_OBJ := ../build/ServingRuntime.o
SHORTHAND_SERVING_LIB := ../build/libshorthand_serving.a
SHORTHAND_SERVING_WORKER_BIN := ../build/shorthand_serving_worker
RUNTIME_ABI_RENAME_DEFS := \
	-Dshort_runtime_reset=shimpl_runtime_reset \
	-Dshort_runtime_model_count=shimpl_runtime_model_count \
	-Dshort_runtime_tensor_count=shimpl_runtime_tensor_count \
	-Dshort_runtime_contract_count=shimpl_runtime_contract_count \
	-Dshort_runtime_measurement_count=shimpl_runtime_measurement_count \
	-Dshort_runtime_infer_count=shimpl_runtime_infer_count \
	-Dshort_runtime_infer_success_count=shimpl_runtime_infer_success_count \
	-Dshort_runtime_infer_not_executed_count=shimpl_runtime_infer_not_executed_count \
	-Dshort_runtime_infer_backend_unavailable_count=shimpl_runtime_infer_backend_unavailable_count \
	-Dshort_runtime_infer_invalid_input_count=shimpl_runtime_infer_invalid_input_count \
	-Dshort_runtime_last_infer_status=shimpl_runtime_last_infer_status \
	-Dshort_runtime_last_infer_backend=shimpl_runtime_last_infer_backend \
	-Dshort_runtime_last_infer_reason=shimpl_runtime_last_infer_reason \
	-Dshort_runtime_last_infer_telemetry_json=shimpl_runtime_last_infer_telemetry_json \
	-Dshort_runtime_infer_bridge_request_json=shimpl_runtime_infer_bridge_request_json \
	-Dshort_runtime_observability_json=shimpl_runtime_observability_json \
	-Dshort_runtime_prometheus_metrics=shimpl_runtime_prometheus_metrics \
	-Dshort_runtime_otlp_spans_json=shimpl_runtime_otlp_spans_json \
	-Dshort_ai_register_model=shimpl_ai_register_model \
	-Dshort_ai_register_tensor=shimpl_ai_register_tensor \
	-Dshort_greenai_register_contract=shimpl_greenai_register_contract \
	-Dshort_greenai_record_measurement=shimpl_greenai_record_measurement \
	-Dshort_ai_infer=shimpl_ai_infer \
	-Dshort_ai_infer_f32=shimpl_ai_infer_f32 \
	-Dshort_ai_infer_legacy=shimpl_ai_infer_legacy

all: compiler

compiler: short_hand

patch-generated-parser:
	@python_not_required=1; \
	if grep -q 'extern "C" void yyerror(char \*s);' parser.tab.cc; then \
		sed -i.bak 's/extern "C" void yyerror(char \*s);/extern "C" void yyerror(char const *s);/' parser.tab.cc; \
		rm -f parser.tab.cc.bak; \
	fi; \
	if grep -q '^void yyerror (char const \*s)' parser.tab.cc; then \
		sed -i.bak 's/^void yyerror (char const \*s)/extern "C" void yyerror (char const *s)/' parser.tab.cc; \
		rm -f parser.tab.cc.bak; \
	fi

runtime-source-lowering: visitors/IR_Generator.cpp ../../../scripts/apply_external_runtime_to_ir_source.sh
	bash ../../../scripts/apply_external_runtime_to_ir_source.sh visitors/IR_Generator.cpp

parser.tab.cc parser.tab.hh: scanner_parser/parser.yy ast/AST.h
	bison -Werror=conflicts-sr -Werror=conflicts-rr --debug -v -d scanner_parser/parser.yy -o parser.tab.cc

lex.yy.c: scanner_parser/scanner.ll parser.tab.hh
	@if command -v flex >/dev/null 2>&1; then flex -o lex.yy.c scanner_parser/scanner.ll; else echo "warning: flex unavailable; using checked-in lex.yy.c"; fi

short_hand: patch-generated-parser runtime-source-lowering $(SRC)
	@mkdir -p ../build
	@if [ -z "$(LLVM_CXXFLAGS)" ] || [ -z "$(LLVM_LDFLAGS)" ]; then \
		echo "error: llvm-config not found or LLVM flags unavailable. Install LLVM with llvm-config in PATH."; \
		exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(AI_CXXFLAGS) $(LLVM_CXXFLAGS) $(ONNX_EXCEPTION_CXXFLAGS) -o $(BIN) $(SRC) $(LDFLAGS) $(LDLIBS) $(AI_LDLIBS) $(LLVM_LDFLAGS)

runtime_lib: $(SHORTHAND_RUNTIME_LIB)

$(SHORTHAND_RUNTIME_LIB): $(SHORTHAND_RUNTIME_SRC) $(SHORTHAND_RUNTIME_FACADE_SRC) runtime/ShorthandRuntime.h
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) $(RUNTIME_ABI_RENAME_DEFS) -c $(SHORTHAND_RUNTIME_SRC) -o $(SHORTHAND_RUNTIME_OBJ)
	$(CXX) $(CXXFLAGS) -c $(SHORTHAND_RUNTIME_FACADE_SRC) -o $(SHORTHAND_RUNTIME_FACADE_OBJ)
	ar rcs $(SHORTHAND_RUNTIME_LIB) $(SHORTHAND_RUNTIME_OBJ) $(SHORTHAND_RUNTIME_FACADE_OBJ)

serving_runtime_lib: $(SHORTHAND_SERVING_LIB)

$(SHORTHAND_SERVING_LIB): $(SHORTHAND_SERVING_SRC) serving/ServingRuntime.h
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) -pthread -c $(SHORTHAND_SERVING_SRC) -o $(SHORTHAND_SERVING_OBJ)
	ar rcs $(SHORTHAND_SERVING_LIB) $(SHORTHAND_SERVING_OBJ)

serving_worker: $(SHORTHAND_SERVING_WORKER_BIN)

$(SHORTHAND_SERVING_WORKER_BIN): $(SHORTHAND_SERVING_WORKER_SRC) $(SHORTHAND_SERVING_LIB)
	$(CXX) $(CXXFLAGS) -pthread $(SHORTHAND_SERVING_WORKER_SRC) $(SHORTHAND_SERVING_LIB) -o $(SHORTHAND_SERVING_WORKER_BIN)

ai_app: ai_app.cpp $(AI_RUNTIME_SRC)
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) $(AI_CXXFLAGS) -o $(AI_APP_BIN) ai_app.cpp $(AI_RUNTIME_CORE_SRC) $(LDFLAGS) $(AI_LDLIBS)

ai_train: ai_train.cpp $(AI_TRAIN_SRC)
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) $(AI_CXXFLAGS) -o $(AI_TRAIN_BIN) ai_train.cpp $(AI_TRAIN_SRC) $(LDFLAGS) $(AI_LDLIBS)

green_ai_tool: $(GREEN_AI_TOOL_SRC)
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) -o $(GREEN_AI_TOOL_BIN) $(GREEN_AI_TOOL_SRC)

c3eco_measurement_tool: $(C3ECO_MEASURE_SRC)
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) -o $(C3ECO_MEASURE_BIN) $(C3ECO_MEASURE_SRC)

test-green: green_ai_tool
	bash ../../../tests/green_ai/test_green_ai_tool.sh

c3eco_gate: green_ai_tool
	../build/green_ai_tool validate ../../../examples/green_ai/image_classification.greenai --strict strict
	../build/green_ai_tool report ../../../examples/green_ai/image_classification.greenai --output /tmp/short_hand_c3eco_report.json --strict strict
	grep -q "Evidence report only; this tool does not grant certification." /tmp/short_hand_c3eco_report.json

test-c3eco: c3eco_gate short_hand
	../build/green_ai_tool check ../../../examples/green_ai/image_classification.greenai --baseline /tmp/short_hand_c3eco_report.json --threshold-percent 10
	SHORTHAND_BIN=$(BIN) bash ../../../scripts/check_c3eco_language_blocks.sh

test-c3eco-profile: short_hand
	SHORTHAND_BIN=$(BIN) bash ../../../scripts/check_c3eco_certification_profile.sh

test-c3eco-measurement: c3eco_measurement_tool
	SHORTHAND_C3ECO_MEASURE_BIN=$(C3ECO_MEASURE_BIN) bash ../../../scripts/check_c3eco_measurement_workbook.sh

regen:
	bison -Werror=conflicts-sr -Werror=conflicts-rr --debug -v -d scanner_parser/parser.yy -o parser.tab.cc
	@if command -v flex >/dev/null 2>&1; then flex -o lex.yy.c scanner_parser/scanner.ll; else echo "warning: flex unavailable; using checked-in lex.yy.c"; fi

.PHONY: all compiler short_hand runtime-source-lowering runtime_lib serving_runtime_lib serving_worker ai_app ai_train green_ai_tool c3eco_measurement_tool c3eco_gate test test-governance test-unit test-integration test-green test-c3eco test-c3eco-profile test-c3eco-measurement test-compiler test-negative test-diagnostics test-runtime-linking test-conformance test-semantic-differential test-control-flow test-enterprise-language test-serving test-fuzz-sanitizers test-tsan test-llvm test-ai test-ai-app sanitize format-check regen patch-generated-parser clean distclean
clean:
	rm -f ../build/short_hand ../build/short_ai_app ../build/short_ai_train ../build/green_ai_tool ../build/shorthand_c3eco_measure ../build/shorthand_serving_worker ../build/libshorthand_runtime.a ../build/libshorthand_serving.a ../build/ShorthandRuntime.o ../build/ShorthandRuntimeImpl.o ../build/RuntimeThreadSafeFacade.o ../build/ServingRuntime.o *.bc *.ir *.o

distclean: clean
	rm -f lex.yy.c parser.tab.cc parser.tab.hh parser.output

test: test-governance test-unit test-integration test-green test-c3eco test-c3eco-profile test-c3eco-measurement test-compiler test-negative test-diagnostics test-runtime-linking test-conformance test-semantic-differential test-control-flow test-enterprise-language test-serving test-llvm test-ai

test-governance:
	bash ../../../scripts/check_production_truth.sh
	bash ../../../tests/governance/test_production_truth_negative.sh
	bash ../../../scripts/check_production_type_memory_model.sh
	bash ../../../scripts/check_functions_control_error_semantics.sh
	bash ../../../scripts/check_enterprise_packages_stdlib_ffi.sh
	bash ../../../scripts/check_concurrent_serving_runtime.sh
	bash ../../../scripts/check_c3eco_certification_profile.sh
	bash ../../../scripts/check_c3eco_measurement_workbook.sh

test-unit: green_ai_tool
	bash ../../../tests/run_language_tests.sh unit

test-integration: short_hand
	bash ../../../scripts/smoke_test.sh

test-compiler: short_hand
	bash ../../../tests/run_language_tests.sh compiler

test-negative: short_hand
	bash ../../../tests/run_language_tests.sh negative

test-diagnostics: short_hand
	SHORTHAND_BIN=$(BIN) bash ../../../tests/diagnostics/test_source_diagnostics.sh

test-runtime-linking: runtime_lib
	SHORTHAND_RUNTIME_LIB=$(SHORTHAND_RUNTIME_LIB) bash ../../../tests/codegen/test_runtime_library_build.sh

test-conformance: short_hand runtime_lib
	SHORTHAND_BIN=$(BIN) SHORTHAND_RUNTIME_LIB=$(SHORTHAND_RUNTIME_LIB) bash ../../../scripts/check_language_correctness.sh

test-semantic-differential: short_hand runtime_lib
	SHORTHAND_BIN=$(BIN) SHORTHAND_RUNTIME_LIB=$(SHORTHAND_RUNTIME_LIB) bash ../../../scripts/check_semantic_differential.sh

test-control-flow: short_hand runtime_lib
	SHORTHAND_BIN=$(BIN) SHORTHAND_RUNTIME_LIB=$(SHORTHAND_RUNTIME_LIB) bash ../../../scripts/check_functions_control_error_semantics.sh

test-enterprise-language: short_hand
	SHORTHAND_BIN=$(BIN) bash ../../../scripts/check_enterprise_packages_stdlib_ffi.sh

test-serving: serving_worker
	CXX=$(CXX) bash ../../../scripts/check_concurrent_serving_runtime.sh

test-fuzz-sanitizers: short_hand
	bash ../../../scripts/check_fuzz_sanitizers.sh

test-tsan:
	bash ../../../scripts/check_thread_sanitizer.sh

test-llvm: short_hand
	bash ../../../tests/run_language_tests.sh llvm

test-ai: short_hand
	bash ../../../tests/run_language_tests.sh ai

test-ai-app: ai_app
	bash ../../../tests/run_language_tests.sh ai_app

sanitize:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -std=c++17" short_hand green_ai_tool c3eco_measurement_tool runtime_lib
	ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:strict_string_checks=1" UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" bash ../../../tests/run_language_tests.sh sanitize
	ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:strict_string_checks=1" UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" SHORTHAND_BIN=$(BIN) bash ../../../scripts/check_c3eco_language_blocks.sh
	ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:strict_string_checks=1" UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" SHORTHAND_BIN=$(BIN) bash ../../../scripts/check_c3eco_certification_profile.sh
	ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:strict_string_checks=1" UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" SHORTHAND_C3ECO_MEASURE_BIN=$(C3ECO_MEASURE_BIN) bash ../../../scripts/check_c3eco_measurement_workbook.sh

format-check:
	@if ! command -v clang-format >/dev/null 2>&1; then \
	  echo "warning: clang-format is not installed; format check is environment-limited"; \
	elif [ "$${CLANG_FORMAT_STRICT:-0}" = "1" ]; then \
	  clang-format --dry-run --Werror main.cpp ast/*.h ast/*.cpp module/*.h module/*.cpp visitors/*.h visitors/*.cpp ai_runtime/*.h ai_runtime/*.cpp green_ai/*.cpp evidence/*.cpp runtime/*.h runtime/*.cpp semantic_ir/*.h; \
	else \
	  echo "clang-format available; set CLANG_FORMAT_STRICT=1 to enforce full legacy formatting"; \
	fi
