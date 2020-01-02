FLAGS=-O3 -flto -Wall -std=c++14 `llvm-config --cppflags --ldflags --system-libs --libs core`

short_hand: ./scanner_parser/* ./ast/* ./visitors/* 
		bison --debug -v -d ./scanner_parser/parser.yy
		flex ./scanner_parser/scanner.ll
		g++ -o ../build/short_hand main.cpp lex.yy.c parser.tab.cc -lfl -ly $(FLAGS) -g -w

.PHONY: clean
clean:
		rm -rf ./build/* lex.yy.c *.tab.*
