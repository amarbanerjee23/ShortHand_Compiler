CXX ?= g++
LLVM_CONFIG ?= llvm-config
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags 2>/dev/null)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --system-libs --libs core bitwriter 2>/dev/null)
CXXFLAGS ?= -O3 -flto -Wall -std=c++17 -g -w
LDFLAGS ?=
LDLIBS ?= -lfl
ONNXRUNTIME_ROOT ?=
LIBTORCH_ROOT ?=

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

# parser.tab.cc currently includes ast/AST.cpp through the committed generated parser.
# Do not also compile ast/AST.cpp here, or the linker can see duplicate AST symbols.
SRC := main.cpp parser.tab.cc lex.yy.c visitors/AST_Printer.cpp visitors/Interpreter.cpp visitors/IR_Generator.cpp ai_runtime/AI_Runtime.cpp
BIN := ../build/short_hand
AI_RUNTIME_SRC := ai_runtime/AI_Runtime.cpp
AI_TRAIN_SRC := ai_runtime/TorchTrainer.cpp
AI_APP_BIN := ../build/short_ai_app
AI_TRAIN_BIN := ../build/short_ai_train
GREEN_AI_TOOL_BIN := ../build/green_ai_tool
GREEN_AI_TOOL_SRC := green_ai/GreenAITool.cpp

all: short_hand

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
	bison --debug -v -d ./scanner_parser/parser.yy
	flex ./scanner_parser/scanner.ll

.PHONY: all short_hand ai_app ai_train green_ai_tool test-green regen patch-generated-parser clean distclean
clean:
	rm -f ../build/short_hand ../build/short_ai_app ../build/short_ai_train ../build/green_ai_tool *.bc *.ir *.o

distclean: clean
	rm -f lex.yy.c parser.tab.cc parser.tab.hh parser.output
