cmake_minimum_required(VERSION 3.16)
project(ShortHand LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)
option(SHORTHAND_ENABLE_ONNXRUNTIME "Enable ONNX Runtime backend" OFF)
option(SHORTHAND_ENABLE_TENSORRT "Enable TensorRT backend" OFF)
option(SHORTHAND_ENABLE_OPENVINO "Enable OpenVINO backend" OFF)
option(SHORTHAND_ENABLE_LIBTORCH "Enable LibTorch backend" OFF)
option(SHORTHAND_ENABLE_LLAMACPP "Enable llama.cpp backend" OFF)
option(SHORTHAND_ENABLE_OPENBLAS "Enable OpenBLAS backend" OFF)
option(SHORTHAND_STRICT_OPTIONAL_BACKENDS "Fail configure when a requested optional backend root is missing" OFF)


function(shorthand_optional_root option_name env_name macro_name include_suffix lib_suffix lib_name)
  set(${macro_name} 0 PARENT_SCOPE)
  if(${option_name})
    if(DEFINED ENV{${env_name}} AND EXISTS "$ENV{${env_name}}")
      set(root "$ENV{${env_name}}")
      message(STATUS "${env_name} root: ${root}")
      set(${macro_name} 1 PARENT_SCOPE)
      if(NOT "${include_suffix}" STREQUAL "")
        set(${macro_name}_INCLUDE "${root}/${include_suffix}" PARENT_SCOPE)
      endif()
      if(NOT "${lib_suffix}" STREQUAL "")
        set(${macro_name}_LIBDIR "${root}/${lib_suffix}" PARENT_SCOPE)
      endif()
      set(${macro_name}_LIB "${lib_name}" PARENT_SCOPE)
    else()
      set(msg "${option_name}=ON but ${env_name} is not set or does not exist; building ${macro_name}=0 fallback stub")
      if(SHORTHAND_STRICT_OPTIONAL_BACKENDS)
        message(FATAL_ERROR "${msg}")
      else()
        message(WARNING "${msg}")
      endif()
    endif()
  endif()
endfunction()

shorthand_optional_root(SHORTHAND_ENABLE_ONNXRUNTIME ONNXRUNTIME_ROOT SHORTHAND_HAS_ONNXRUNTIME include lib onnxruntime)
if(SHORTHAND_ENABLE_TENSORRT)
  if(DEFINED ENV{TENSORRT_ROOT} AND EXISTS "$ENV{TENSORRT_ROOT}" AND DEFINED ENV{CUDA_ROOT} AND EXISTS "$ENV{CUDA_ROOT}")
    set(SHORTHAND_HAS_TENSORRT 1)
    set(SHORTHAND_HAS_TENSORRT_INCLUDE "$ENV{TENSORRT_ROOT}/include")
    set(SHORTHAND_HAS_TENSORRT_LIBDIR "$ENV{TENSORRT_ROOT}/lib")
    message(STATUS "TensorRT root: $ENV{TENSORRT_ROOT}; CUDA root: $ENV{CUDA_ROOT}")
  else()
    set(SHORTHAND_HAS_TENSORRT 0)
    set(msg "SHORTHAND_ENABLE_TENSORRT=ON requires TENSORRT_ROOT and CUDA_ROOT; building TensorRT fallback stub")
    if(SHORTHAND_STRICT_OPTIONAL_BACKENDS)
      message(FATAL_ERROR "${msg}")
    else()
      message(WARNING "${msg}")
    endif()
  endif()
else()
  set(SHORTHAND_HAS_TENSORRT 0)
endif()
shorthand_optional_root(SHORTHAND_ENABLE_OPENVINO OPENVINO_ROOT SHORTHAND_HAS_OPENVINO include lib openvino)
shorthand_optional_root(SHORTHAND_ENABLE_LIBTORCH LIBTORCH_ROOT SHORTHAND_HAS_LIBTORCH include lib torch)
shorthand_optional_root(SHORTHAND_ENABLE_LLAMACPP LLAMACPP_ROOT SHORTHAND_HAS_LLAMACPP include lib llama)
shorthand_optional_root(SHORTHAND_ENABLE_OPENBLAS OPENBLAS_ROOT SHORTHAND_HAS_OPENBLAS include lib openblas)
if(DEFINED ENV{EIGEN_ROOT} AND EXISTS "$ENV{EIGEN_ROOT}")
  set(SHORTHAND_HAS_EIGEN 1)
else()
  set(SHORTHAND_HAS_EIGEN 0)
endif()

find_package(BISON REQUIRED)
find_package(FLEX QUIET)
find_program(LLVM_CONFIG llvm-config)
find_program(BASH_EXECUTABLE bash REQUIRED)

if(LLVM_CONFIG)
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
else()
  message(WARNING "llvm-config not found; short_hand target may require LLVM in full builds")
  set(LLVM_CXXFLAGS "")
  set(LLVM_LDFLAGS "")
endif()
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

if(FLEX_EXECUTABLE)
add_custom_command(
  OUTPUT ${SCANNER_SRC}
  COMMAND ${FLEX_EXECUTABLE} -o ${SCANNER_SRC} ${SRC_DIR}/scanner_parser/scanner.ll
  DEPENDS ${SRC_DIR}/scanner_parser/scanner.ll ${PARSER_HDR}
  COMMENT "Generating ShortHand scanner"
  VERBATIM
)
else()
  set(SCANNER_SRC ${SRC_DIR}/lex.yy.c)
  message(WARNING "flex not found; using checked-in lex.yy.c")
endif()

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
  ${SRC_DIR}/visitors/SemanticAnalyzer.cpp
  ${SRC_DIR}/visitors/Diagnostics.cpp
  ${SRC_DIR}/evidence/EvidenceEmitter.cpp
  ${SRC_DIR}/ai_runtime/AI_Runtime.cpp
  ${SRC_DIR}/ai_runtime/AI_Backend.cpp
  ${SRC_DIR}/ai_runtime/AI_Types.cpp
  ${SRC_DIR}/ai_runtime/backends/FallbackBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/OnnxRuntimeBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/TensorRTBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/OpenVINOBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/LibTorchBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/LlamaCppBackend.cpp
)
target_include_directories(short_hand PRIVATE ${SRC_DIR} ${GENERATED_DIR})
target_compile_options(short_hand PRIVATE ${LLVM_CXXFLAGS})
target_compile_definitions(short_hand PRIVATE SHORTHAND_HAS_ONNXRUNTIME=${SHORTHAND_HAS_ONNXRUNTIME} SHORTHAND_HAS_TENSORRT=${SHORTHAND_HAS_TENSORRT} SHORTHAND_HAS_OPENVINO=${SHORTHAND_HAS_OPENVINO} SHORTHAND_HAS_LIBTORCH=${SHORTHAND_HAS_LIBTORCH} SHORTHAND_HAS_LLAMACPP=${SHORTHAND_HAS_LLAMACPP} SHORTHAND_HAS_OPENBLAS=${SHORTHAND_HAS_OPENBLAS} SHORTHAND_HAS_EIGEN=${SHORTHAND_HAS_EIGEN})
if(SHORTHAND_HAS_ONNXRUNTIME)
  target_include_directories(short_hand PRIVATE ${SHORTHAND_HAS_ONNXRUNTIME_INCLUDE})
  target_link_directories(short_hand PRIVATE ${SHORTHAND_HAS_ONNXRUNTIME_LIBDIR})
  target_link_libraries(short_hand PRIVATE onnxruntime)
endif()
if(SHORTHAND_HAS_TENSORRT)
  target_include_directories(short_hand PRIVATE ${SHORTHAND_HAS_TENSORRT_INCLUDE})
  target_link_directories(short_hand PRIVATE ${SHORTHAND_HAS_TENSORRT_LIBDIR})
endif()
if(SHORTHAND_HAS_OPENVINO)
  target_include_directories(short_hand PRIVATE ${SHORTHAND_HAS_OPENVINO_INCLUDE})
  target_link_directories(short_hand PRIVATE ${SHORTHAND_HAS_OPENVINO_LIBDIR})
endif()
if(SHORTHAND_HAS_LIBTORCH)
  target_include_directories(short_hand PRIVATE ${SHORTHAND_HAS_LIBTORCH_INCLUDE})
  target_link_directories(short_hand PRIVATE ${SHORTHAND_HAS_LIBTORCH_LIBDIR})
endif()
if(SHORTHAND_HAS_LLAMACPP)
  target_include_directories(short_hand PRIVATE ${SHORTHAND_HAS_LLAMACPP_INCLUDE})
  target_link_directories(short_hand PRIVATE ${SHORTHAND_HAS_LLAMACPP_LIBDIR})
endif()
target_link_libraries(short_hand PRIVATE ${LLVM_LDFLAGS})

find_library(FL_LIBRARY fl)
if(FLEX_LIBRARIES)
  target_link_libraries(short_hand PRIVATE ${FLEX_LIBRARIES})
elseif(FL_LIBRARY)
  target_link_libraries(short_hand PRIVATE ${FL_LIBRARY})
endif()

add_executable(green_ai_tool ${SRC_DIR}/green_ai/GreenAITool.cpp)


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
