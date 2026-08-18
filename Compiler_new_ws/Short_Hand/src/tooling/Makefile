CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=
BIN ?= ../../build/shorthand_tool
SOURCES := SourceTools.cpp SourceToolMain.cpp

.PHONY: all clean

all: $(BIN)

$(BIN): $(SOURCES) SourceTools.h
	@mkdir -p "$(dir $(BIN))"
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o "$(BIN)"

clean:
	rm -f "$(BIN)"
