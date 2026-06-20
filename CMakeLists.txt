cmake_minimum_required(VERSION 3.16)
project(ShortHand LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_program(LLVM_CONFIG llvm-config REQUIRED)

execute_process(COMMAND ${LLVM_CONFIG} --cxxflags OUTPUT_VARIABLE LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG} --ldflags --system-libs --libs core bitwriter OUTPUT_VARIABLE LLVM_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(LLVM_CXXFLAGS)
separate_arguments(LLVM_LDFLAGS)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Compiler_new_ws/Short_Hand/src)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

BISON_TARGET(ShortHandParser
  ${SRC_DIR}/scanner_parser/parser.yy
  ${GENERATED_DIR}/parser.tab.cc
  DEFINES_FILE ${GENERATED_DIR}/parser.tab.hh
  COMPILE_FLAGS "-Werror=conflicts-sr -Werror=conflicts-rr --debug -v")
FLEX_TARGET(ShortHandScanner
  ${SRC_DIR}/scanner_parser/scanner.ll
  ${GENERATED_DIR}/lex.yy.c)
ADD_FLEX_BISON_DEPENDENCY(ShortHandScanner ShortHandParser)

add_executable(short_hand
  ${SRC_DIR}/main.cpp
  ${BISON_ShortHandParser_OUTPUTS}
  ${FLEX_ShortHandScanner_OUTPUTS}
  ${SRC_DIR}/ast/AST.cpp
  ${SRC_DIR}/visitors/AST_Printer.cpp
  ${SRC_DIR}/visitors/Interpreter.cpp
  ${SRC_DIR}/visitors/IR_Generator.cpp
  ${SRC_DIR}/ai_runtime/AI_Runtime.cpp)
target_include_directories(short_hand PRIVATE ${SRC_DIR} ${GENERATED_DIR})
target_compile_options(short_hand PRIVATE ${LLVM_CXXFLAGS})
target_link_libraries(short_hand PRIVATE ${LLVM_LDFLAGS} fl)

add_executable(green_ai_tool ${SRC_DIR}/green_ai/GreenAITool.cpp)

if(DEFINED ENV{ONNXRUNTIME_ROOT})
  message(STATUS "ONNX Runtime root: $ENV{ONNXRUNTIME_ROOT}")
else()
  message(STATUS "ONNXRUNTIME_ROOT not set; optional AI runtime target not enabled")
endif()
if(DEFINED ENV{LIBTORCH_ROOT})
  message(STATUS "LibTorch root: $ENV{LIBTORCH_ROOT}")
else()
  message(STATUS "LIBTORCH_ROOT not set; optional training demo target not enabled")
endif()

enable_testing()
add_test(NAME green_ai_tool COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/green_ai/test_green_ai_tool.sh)
add_test(NAME cmake_short_hand_greenai COMMAND short_hand ${CMAKE_CURRENT_SOURCE_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short run)
set_tests_properties(cmake_short_hand_greenai PROPERTIES PASS_REGULAR_EXPRESSION "GreenAI workload")
