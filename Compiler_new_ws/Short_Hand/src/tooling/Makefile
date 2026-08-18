CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=
BIN ?= ../../build/shorthand_tool
LSP_BIN ?= ../../build/shorthand_lsp
SOURCES := SourceTools.cpp SourceToolMain.cpp
LSP_SOURCES := LanguageServerMain.cpp

.PHONY: all lsp clean

all: $(BIN)

lsp: $(LSP_BIN)

$(BIN): $(SOURCES) SourceTools.h
	@mkdir -p "$(dir $(BIN))"
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o "$(BIN)"

$(LSP_BIN): $(LSP_SOURCES)
	@mkdir -p "$(dir $(LSP_BIN))"
	$(CXX) $(CXXFLAGS) $(LSP_SOURCES) $(LDFLAGS) -o "$(LSP_BIN)"

clean:
	rm -f "$(BIN)" "$(LSP_BIN)"
