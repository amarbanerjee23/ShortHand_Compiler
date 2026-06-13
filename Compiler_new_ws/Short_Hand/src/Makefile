CXX ?= g++
LLVM_CONFIG ?= llvm-config
LLVM_FLAGS := $(shell $(LLVM_CONFIG) --cppflags --ldflags --system-libs --libs core 2>/dev/null)
CXXFLAGS ?= -O3 -flto -Wall -std=c++14 -g -w
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

SRC := main.cpp parser.tab.cc lex.yy.c ast/AST.cpp visitors/AST_Printer.cpp visitors/Interpreter.cpp visitors/IR_Generator.cpp ai_runtime/AI_Runtime.cpp
BIN := ../build/short_hand
AI_RUNTIME_SRC := ai_runtime/AI_Runtime.cpp
AI_TRAIN_SRC := ai_runtime/TorchTrainer.cpp
AI_APP_BIN := ../build/short_ai_app
AI_TRAIN_BIN := ../build/short_ai_train

all: short_hand

short_hand: $(SRC)
	@mkdir -p ../build
	@if [ -z "$(LLVM_FLAGS)" ]; then \
		echo "error: llvm-config not found. Install LLVM (with llvm-config in PATH) to build the compiler."; \
		exit 1; \
	fi
	$(CXX) $(CXXFLAGS) -o $(BIN) $(SRC) $(LDFLAGS) $(LDLIBS) $(LLVM_FLAGS)

ai_app: ai_app.cpp $(AI_RUNTIME_SRC)
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) $(AI_CXXFLAGS) -o $(AI_APP_BIN) ai_app.cpp $(AI_RUNTIME_SRC) $(LDFLAGS) $(AI_LDLIBS)

ai_train: ai_train.cpp $(AI_TRAIN_SRC)
	@mkdir -p ../build
	$(CXX) $(CXXFLAGS) $(AI_CXXFLAGS) -o $(AI_TRAIN_BIN) ai_train.cpp $(AI_TRAIN_SRC) $(LDFLAGS) $(AI_LDLIBS)

regen:
	bison --debug -v -d ./scanner_parser/parser.yy
	flex ./scanner_parser/scanner.ll

.PHONY: all short_hand ai_app ai_train regen clean
clean:
	rm -f ../build/short_hand ../build/short_ai_app ../build/short_ai_train lex.yy.c parser.tab.cc parser.tab.hh parser.output
