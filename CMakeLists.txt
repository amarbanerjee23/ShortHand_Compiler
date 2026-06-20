cmake_minimum_required(VERSION 3.16)
project(ShortHand LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_program(LLVM_CONFIG llvm-config REQUIRED)
find_program(BASH_EXECUTABLE bash REQUIRED)

execute_process(
  COMMAND ${LLVM_CONFIG} --cxxflags
  OUTPUT_VARIABLE LLVM_CXXFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${LLVM_CONFIG} --ldflags --system-libs --libs core bitwriter
  OUTPUT_VARIABLE LLVM_LDFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_CXXFLAGS NATIVE_COMMAND "${LLVM_CXXFLAGS}")
separate_arguments(LLVM_LDFLAGS NATIVE_COMMAND "${LLVM_LDFLAGS}")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Compiler_new_ws/Short_Hand/src)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(PARSER_SRC ${GENERATED_DIR}/parser.tab.cc)
set(PARSER_HDR ${GENERATED_DIR}/parser.tab.hh)
set(SCANNER_SRC ${GENERATED_DIR}/lex.yy.cpp)

add_custom_command(
  OUTPUT ${PARSER_SRC} ${PARSER_HDR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
  COMMAND ${BISON_EXECUTABLE} -Werror=conflicts-sr -Werror=conflicts-rr --debug -v -d
          ${SRC_DIR}/scanner_parser/parser.yy
          -o ${PARSER_SRC}
  DEPENDS ${SRC_DIR}/scanner_parser/parser.yy ${SRC_DIR}/ast/AST.h
  COMMENT "Generating ShortHand parser"
  VERBATIM
)

add_custom_command(
  OUTPUT ${SCANNER_SRC}
  COMMAND ${FLEX_EXECUTABLE} -o ${SCANNER_SRC} ${SRC_DIR}/scanner_parser/scanner.ll
  DEPENDS ${SRC_DIR}/scanner_parser/scanner.ll ${PARSER_HDR}
  COMMENT "Generating ShortHand scanner"
  VERBATIM
)

set_source_files_properties(
  ${PARSER_SRC}
  ${SCANNER_SRC}
  PROPERTIES GENERATED TRUE
)

set_source_files_properties(
  ${SCANNER_SRC}
  PROPERTIES LANGUAGE CXX
)

add_executable(short_hand
  ${SRC_DIR}/main.cpp
  ${PARSER_SRC}
  ${SCANNER_SRC}
  ${SRC_DIR}/ast/AST.cpp
  ${SRC_DIR}/visitors/AST_Printer.cpp
  ${SRC_DIR}/visitors/Interpreter.cpp
  ${SRC_DIR}/visitors/IR_Generator.cpp
  ${SRC_DIR}/ai_runtime/AI_Runtime.cpp
)
target_include_directories(short_hand PRIVATE ${SRC_DIR} ${GENERATED_DIR})
target_compile_options(short_hand PRIVATE ${LLVM_CXXFLAGS})
target_link_libraries(short_hand PRIVATE ${LLVM_LDFLAGS})

find_library(FL_LIBRARY fl)
if(FLEX_LIBRARIES)
  target_link_libraries(short_hand PRIVATE ${FLEX_LIBRARIES})
elseif(FL_LIBRARY)
  target_link_libraries(short_hand PRIVATE ${FL_LIBRARY})
endif()

add_executable(green_ai_tool ${SRC_DIR}/green_ai/GreenAITool.cpp)

if(DEFINED ENV{ONNXRUNTIME_ROOT})
  message(STATUS "ONNX Runtime root: $ENV{ONNXRUNTIME_ROOT}")
else()
  message(STATUS "ONNXRUNTIME_ROOT not set; optional AI runtime integration is not built by CMake")
endif()

if(DEFINED ENV{LIBTORCH_ROOT})
  message(STATUS "LibTorch root: $ENV{LIBTORCH_ROOT}")
else()
  message(STATUS "LIBTORCH_ROOT not set; optional training demo is not built by CMake")
endif()

enable_testing()

add_test(
  NAME green_ai_tool_tests
  COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/green_ai/test_green_ai_tool.sh
)
set_tests_properties(green_ai_tool_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(
  NAME cmake_short_hand_greenai_run
  COMMAND $<TARGET_FILE:short_hand> ${CMAKE_CURRENT_SOURCE_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short run
)
set_tests_properties(cmake_short_hand_greenai_run PROPERTIES
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PASS_REGULAR_EXPRESSION "GreenAI workload"
)
