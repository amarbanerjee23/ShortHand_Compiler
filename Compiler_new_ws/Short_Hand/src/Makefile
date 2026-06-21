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

AI_CXXFLAGS :=
AI_LDLIBS :=

ifneq ($(ONNXRUNTIME_ROOT),)
AI_CXXFLAGS += -DUSE_ONNXRUNTIME -I$(ONNXRUNTIME_ROOT)/include
AI_LDLIBS += -L$(ONNXRUNTIME_ROOT)/lib -lonnxruntime
endif

ifneq ($(LIBTORCH_ROOT),)
AI_CXXFLAGS += -DUSE_LIBTORCH -I$(LIBTORCH_ROOT)/include -I$(LIBTORCH_ROOT)/include/torch/csrc/api/include
AI_LDLIBS += -L$(LIBTORCH_ROOT)/lib -ltorch -ltorch_cpu -lc10
endif

SRC := main.cpp parser.tab.cc lex.yy.c ast/AST.cpp visitors/AST_Printer.cpp visitors/Interpreter.cpp visitors/IR_Generator.cpp visitors/SemanticAnalyzer.cpp visitors/Diagnostics.cpp evidence/EvidenceEmitter.cpp ai_runtime/AI_Runtime.cpp ai_runtime/AI_Backend.cpp ai_runtime/backends/FallbackBackend.cpp ai_runtime/backends/OnnxRuntimeBackend.cpp ai_runtime/backends/TensorRTBackend.cpp ai_runtime/backends/OpenVINOBackend.cpp ai_runtime/backends/LibTorchBackend.cpp ai_runtime/backends/LlamaCppBackend.cpp
BIN := ../build/short_hand
AI_RUNTIME_SRC := ai_runtime/AI_Runtime.cpp
AI_TRAIN_SRC := ai_runtime/TorchTrainer.cpp
AI_APP_BIN := ../build/short_ai_app
AI_TRAIN_BIN := ../build/short_ai_train
GREEN_AI_TOOL_BIN := ../build/green_ai_tool
GREEN_AI_TOOL_SRC := green_ai/GreenAITool.cpp

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

parser.tab.cc parser.tab.hh: scanner_parser/parser.yy ast/AST.h
	bison -Werror=conflicts-sr -Werror=conflicts-rr --debug -v -d scanner_parser/parser.yy -o parser.tab.cc

lex.yy.c: scanner_parser/scanner.ll parser.tab.hh
	@if command -v flex >/dev/null 2>&1; then flex -o lex.yy.c scanner_parser/scanner.ll; else echo "warning: flex unavailable; using checked-in lex.yy.c"; fi

short_hand: patch-generated-parser $(SRC)
	@mkdir -p ../build
	@if [ -z "$(LLVM_CXXFLAGS)" ] || [ -z "$(LLVM_LDFLAGS)" ]; then \
		echo "error: llvm-config not found or LLVM flags unavailable. Install LLVM with llvm-config in PATH."; \
		exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) -o $(BIN) $(SRC) $(LDFLAGS) $(LDLIBS) $(LLVM_LDFLAGS)

ai_app: ai_app.cpp $(AI_RUNTIME_SRC)
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) $(AI_CXXFLAGS) -o $(AI_APP_BIN) ai_app.cpp $(AI_RUNTIME_SRC) $(LDFLAGS) $(AI_LDLIBS)

ai_train: ai_train.cpp $(AI_TRAIN_SRC)
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) $(AI_CXXFLAGS) -o $(AI_TRAIN_BIN) ai_train.cpp $(AI_TRAIN_SRC) $(LDFLAGS) $(AI_LDLIBS)

green_ai_tool: $(GREEN_AI_TOOL_SRC)
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) -o $(GREEN_AI_TOOL_BIN) $(GREEN_AI_TOOL_SRC)

test-green: green_ai_tool
	bash ../../../tests/green_ai/test_green_ai_tool.sh

regen:
	bison -Werror=conflicts-sr -Werror=conflicts-rr --debug -v -d scanner_parser/parser.yy -o parser.tab.cc
	@if command -v flex >/dev/null 2>&1; then flex -o lex.yy.c scanner_parser/scanner.ll; else echo "warning: flex unavailable; using checked-in lex.yy.c"; fi

.PHONY: all compiler short_hand ai_app ai_train green_ai_tool test test-unit test-integration test-green test-compiler test-negative test-llvm test-ai sanitize format-check regen patch-generated-parser clean distclean
clean:
	rm -f ../build/short_hand ../build/short_ai_app ../build/short_ai_train ../build/green_ai_tool *.bc *.ir *.o

distclean: clean
	rm -f lex.yy.c parser.tab.cc parser.tab.hh parser.output

test: test-unit test-integration test-green test-compiler test-negative test-llvm test-ai

test-unit: green_ai_tool
	bash ../../../tests/run_language_tests.sh unit

test-integration: short_hand
	bash ../../../scripts/smoke_test.sh

test-compiler: short_hand
	bash ../../../tests/run_language_tests.sh compiler

test-negative: short_hand
	bash ../../../tests/run_language_tests.sh negative

test-llvm: short_hand
	bash ../../../tests/run_language_tests.sh llvm

test-ai: ai_app
	bash ../../../tests/run_language_tests.sh ai

sanitize:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -std=c++17" short_hand green_ai_tool
	bash ../../../tests/run_language_tests.sh sanitize

format-check:
	@if ! command -v clang-format >/dev/null 2>&1; then \
	  echo "warning: clang-format is not installed; format check is environment-limited"; \
	elif [ "$${CLANG_FORMAT_STRICT:-0}" = "1" ]; then \
	  clang-format --dry-run --Werror main.cpp ast/*.h ast/*.cpp visitors/*.h visitors/*.cpp ai_runtime/*.h ai_runtime/*.cpp green_ai/*.cpp; \
	else \
	  echo "clang-format available; set CLANG_FORMAT_STRICT=1 to enforce full legacy formatting"; \
	fi
