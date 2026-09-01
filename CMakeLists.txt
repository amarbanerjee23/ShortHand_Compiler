cmake_minimum_required(VERSION 3.16)
project(ShortHand VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_compile_options(-Wall -Wextra -Wpedantic)
option(SHORTHAND_ENABLE_ONNXRUNTIME "Enable ONNX Runtime backend" OFF)
option(SHORTHAND_ENABLE_TENSORRT "Enable TensorRT backend" OFF)
option(SHORTHAND_ENABLE_OPENVINO "Enable OpenVINO backend" OFF)
option(SHORTHAND_ENABLE_LIBTORCH "Enable LibTorch backend" OFF)
option(SHORTHAND_ENABLE_LLAMACPP "Enable llama.cpp backend" OFF)
option(SHORTHAND_ENABLE_OPENBLAS "Enable OpenBLAS backend" OFF)
option(SHORTHAND_STRICT_OPTIONAL_BACKENDS "Fail when an enabled optional backend root is missing" OFF)
option(SHORTHAND_BUILD_TESTING "Build and register ShortHand tests" ON)
option(SHORTHAND_BUILD_PROMETHEUS_ADAPTER "Build the Prometheus scrape endpoint host adapter" ON)
option(SHORTHAND_BUILD_OTLP_EXPORTER "Build the OTLP/HTTP exporter adapter" ON)
set(ONNXRUNTIME_ROOT "" CACHE PATH "ONNX Runtime root")
set(TENSORRT_ROOT "" CACHE PATH "TensorRT root")
set(CUDA_ROOT "" CACHE PATH "CUDA root")
set(OPENVINO_ROOT "" CACHE PATH "OpenVINO root")
set(LIBTORCH_ROOT "" CACHE PATH "LibTorch root")
set(LLAMACPP_ROOT "" CACHE PATH "llama.cpp root")
set(OPENBLAS_ROOT "" CACHE PATH "OpenBLAS root")
set(EIGEN_ROOT "" CACHE PATH "Eigen root")
function(shorthand_optional name enabled root outvar)
  if(${enabled} AND NOT ${root} STREQUAL "")
    set(${outvar} 1 PARENT_SCOPE)
  elseif(${enabled})
    if(SHORTHAND_STRICT_OPTIONAL_BACKENDS)
      message(FATAL_ERROR "${name} enabled but root is missing")
    else()
      message(WARNING "${name} enabled but root is missing; building unavailable stub")
      set(${outvar} 0 PARENT_SCOPE)
    endif()
  else()
    set(${outvar} 0 PARENT_SCOPE)
  endif()
endfunction()
shorthand_optional("ONNX Runtime" SHORTHAND_ENABLE_ONNXRUNTIME "${ONNXRUNTIME_ROOT}" SHORTHAND_HAS_ONNXRUNTIME)
shorthand_optional("TensorRT" SHORTHAND_ENABLE_TENSORRT "${TENSORRT_ROOT}" SHORTHAND_HAS_TENSORRT)
shorthand_optional("OpenVINO" SHORTHAND_ENABLE_OPENVINO "${OPENVINO_ROOT}" SHORTHAND_HAS_OPENVINO)
shorthand_optional("LibTorch" SHORTHAND_ENABLE_LIBTORCH "${LIBTORCH_ROOT}" SHORTHAND_HAS_LIBTORCH)
shorthand_optional("llama.cpp" SHORTHAND_ENABLE_LLAMACPP "${LLAMACPP_ROOT}" SHORTHAND_HAS_LLAMACPP)
shorthand_optional("OpenBLAS" SHORTHAND_ENABLE_OPENBLAS "${OPENBLAS_ROOT}" SHORTHAND_HAS_OPENBLAS)
set(SHORTHAND_HAS_EIGEN 0)
if(NOT EIGEN_ROOT STREQUAL "")
  set(SHORTHAND_HAS_EIGEN 1)
endif()

find_package(BISON REQUIRED)
find_package(FLEX QUIET)
find_package(Threads REQUIRED)
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

add_custom_target(shorthand_runtime_source_lowering
  COMMAND ${BASH_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/scripts/apply_external_runtime_to_ir_source.sh
          ${SRC_DIR}/visitors/IR_Generator.cpp
  DEPENDS ${SRC_DIR}/visitors/IR_Generator.cpp ${CMAKE_CURRENT_SOURCE_DIR}/scripts/apply_external_runtime_to_ir_source.sh
  COMMENT "Applying source-level external runtime lowering to IR_Generator.cpp"
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

set(SHORTHAND_AI_RUNTIME_SOURCES
  ${SRC_DIR}/ai_runtime/AI_Runtime.cpp
  ${SRC_DIR}/ai_runtime/AI_Types.cpp
  ${SRC_DIR}/ai_runtime/AI_Telemetry.cpp
  ${SRC_DIR}/ai_runtime/AI_Backend.cpp
  ${SRC_DIR}/ai_runtime/backends/FallbackBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/OnnxRuntimeBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/TensorRTBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/OpenVINOBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/LibTorchBackend.cpp
  ${SRC_DIR}/ai_runtime/backends/LlamaCppBackend.cpp
)

set(SHORTHAND_RUNTIME_ABI_VERSION "1.0.0")
set(SHORTHAND_RUNTIME_API_VERSION "1.0.0")
set(SHORTHAND_RUNTIME_ABI_VERSION_MAJOR 1)
set(SHORTHAND_AI_BRIDGE_CONTRACT_VERSION "shorthand.runtime.ai_runtime_execution_adapter.v1")
set(SHORTHAND_CORE_FFI_ABI_VERSION "1.0.0")
set(SHORTHAND_CORE_FFI_ABI_VERSION_MAJOR 1)

set(SHORTHAND_RUNTIME_ABI_RENAMES
  short_runtime_reset=shimpl_runtime_reset
  short_runtime_model_count=shimpl_runtime_model_count
  short_runtime_tensor_count=shimpl_runtime_tensor_count
  short_runtime_contract_count=shimpl_runtime_contract_count
  short_runtime_measurement_count=shimpl_runtime_measurement_count
  short_runtime_infer_count=shimpl_runtime_infer_count
  short_runtime_infer_success_count=shimpl_runtime_infer_success_count
  short_runtime_infer_not_executed_count=shimpl_runtime_infer_not_executed_count
  short_runtime_infer_backend_unavailable_count=shimpl_runtime_infer_backend_unavailable_count
  short_runtime_infer_invalid_input_count=shimpl_runtime_infer_invalid_input_count
  short_runtime_last_infer_status=shimpl_runtime_last_infer_status
  short_runtime_last_infer_backend=shimpl_runtime_last_infer_backend
  short_runtime_last_infer_reason=shimpl_runtime_last_infer_reason
  short_runtime_last_infer_telemetry_json=shimpl_runtime_last_infer_telemetry_json
  short_runtime_infer_bridge_request_json=shimpl_runtime_infer_bridge_request_json
  short_runtime_observability_json=shimpl_runtime_observability_json
  short_runtime_prometheus_metrics=shimpl_runtime_prometheus_metrics
  short_runtime_otlp_spans_json=shimpl_runtime_otlp_spans_json
  short_ai_register_model=shimpl_ai_register_model
  short_ai_register_tensor=shimpl_ai_register_tensor
  short_greenai_register_contract=shimpl_greenai_register_contract
  short_greenai_record_measurement=shimpl_greenai_record_measurement
  short_ai_infer=shimpl_ai_infer
  short_ai_infer_f32=shimpl_ai_infer_f32
  short_ai_infer_legacy=shimpl_ai_infer_legacy
)
set_source_files_properties(
  ${SRC_DIR}/runtime/ShorthandRuntime.cpp
  PROPERTIES COMPILE_DEFINITIONS "${SHORTHAND_RUNTIME_ABI_RENAMES}"
)

set(SHORTHAND_RUNTIME_SOURCES
  ${SRC_DIR}/runtime/ShorthandRuntime.cpp
  ${SRC_DIR}/runtime/RuntimeThreadSafeFacade.cpp
)
set(SHORTHAND_AI_BRIDGE_SOURCES
  ${SRC_DIR}/runtime/AIRuntimeBridgeAdapter.cpp
  ${SRC_DIR}/ai_runtime/AI_Types.cpp
)

function(shorthand_configure_installable_target target export_name)
  target_compile_features(${target} PUBLIC cxx_std_17)
  target_include_directories(${target}
    PUBLIC
      $<BUILD_INTERFACE:${SRC_DIR}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/shorthand>
  )
  set_target_properties(${target} PROPERTIES EXPORT_NAME ${export_name})
endfunction()

add_library(shorthand_runtime STATIC ${SHORTHAND_RUNTIME_SOURCES})
shorthand_configure_installable_target(shorthand_runtime runtime)
target_link_libraries(shorthand_runtime PUBLIC Threads::Threads)
set_target_properties(shorthand_runtime PROPERTIES OUTPUT_NAME shorthand_runtime)

add_library(shorthand_runtime_shared SHARED ${SHORTHAND_RUNTIME_SOURCES})
shorthand_configure_installable_target(shorthand_runtime_shared runtime_shared)
target_link_libraries(shorthand_runtime_shared PUBLIC Threads::Threads)
target_compile_definitions(shorthand_runtime_shared PRIVATE
  SHORTHAND_RUNTIME_SHARED=1
  SHORTHAND_RUNTIME_BUILDING_LIBRARY=1
)
set_target_properties(shorthand_runtime_shared PROPERTIES
  OUTPUT_NAME shorthand_runtime
  VERSION ${SHORTHAND_RUNTIME_ABI_VERSION}
  SOVERSION ${SHORTHAND_RUNTIME_ABI_VERSION_MAJOR}
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)
if(WIN32)
  set_target_properties(shorthand_runtime_shared PROPERTIES OUTPUT_NAME shorthand_runtime_shared)
endif()

add_library(shorthand_ai_bridge STATIC ${SHORTHAND_AI_BRIDGE_SOURCES})
shorthand_configure_installable_target(shorthand_ai_bridge ai_bridge)
set_target_properties(shorthand_ai_bridge PROPERTIES OUTPUT_NAME shorthand_ai_bridge)

add_library(shorthand_ai_bridge_shared SHARED ${SHORTHAND_AI_BRIDGE_SOURCES})
shorthand_configure_installable_target(shorthand_ai_bridge_shared ai_bridge_shared)
set_target_properties(shorthand_ai_bridge_shared PROPERTIES
  OUTPUT_NAME shorthand_ai_bridge
  VERSION ${SHORTHAND_RUNTIME_ABI_VERSION}
  SOVERSION ${SHORTHAND_RUNTIME_ABI_VERSION_MAJOR}
)
if(WIN32)
  set_target_properties(shorthand_ai_bridge_shared PROPERTIES OUTPUT_NAME shorthand_ai_bridge_shared)
endif()

add_library(shorthand_core STATIC ${SRC_DIR}/core/ShorthandCore.cpp)
target_compile_features(shorthand_core PUBLIC cxx_std_17)
target_include_directories(shorthand_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(shorthand_core PROPERTIES EXPORT_NAME core OUTPUT_NAME shorthand_core)

add_library(shorthand_core_shared SHARED ${SRC_DIR}/core/ShorthandCore.cpp)
target_compile_features(shorthand_core_shared PUBLIC cxx_std_17)
target_include_directories(shorthand_core_shared PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(shorthand_core_shared
  PUBLIC SHORTHAND_CORE_SHARED=1
  PRIVATE SHORTHAND_CORE_BUILDING_LIBRARY=1
)
set_target_properties(shorthand_core_shared PROPERTIES
  EXPORT_NAME core_shared
  OUTPUT_NAME shorthand_core
  VERSION ${SHORTHAND_CORE_FFI_ABI_VERSION}
  SOVERSION ${SHORTHAND_CORE_FFI_ABI_VERSION_MAJOR}
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)
if(WIN32)
  set_target_properties(shorthand_core_shared PROPERTIES OUTPUT_NAME shorthand_core_shared)
endif()

add_library(shorthand_serving STATIC ${SRC_DIR}/serving/ServingRuntime.cpp)
shorthand_configure_installable_target(shorthand_serving serving)
target_link_libraries(shorthand_serving PUBLIC Threads::Threads)
set_target_properties(shorthand_serving PROPERTIES OUTPUT_NAME shorthand_serving)

add_executable(shorthand_serving_worker
  ${SRC_DIR}/serving/ServingWorkerMain.cpp
)
target_compile_features(shorthand_serving_worker PRIVATE cxx_std_17)
target_link_libraries(shorthand_serving_worker PRIVATE shorthand_serving)

if(SHORTHAND_BUILD_PROMETHEUS_ADAPTER)
  add_executable(shorthand_prometheus_adapter
    ${SRC_DIR}/operations/PrometheusScrapeAdapter.cpp
  )
  target_include_directories(shorthand_prometheus_adapter PRIVATE ${SRC_DIR})
  target_link_libraries(shorthand_prometheus_adapter PRIVATE shorthand_runtime)
endif()

if(SHORTHAND_BUILD_OTLP_EXPORTER)
  add_executable(shorthand_otlp_exporter
    ${SRC_DIR}/operations/OtlpExporterAdapter.cpp
  )
  target_include_directories(shorthand_otlp_exporter PRIVATE ${SRC_DIR})
  target_link_libraries(shorthand_otlp_exporter PRIVATE shorthand_runtime)
endif()

add_executable(short_hand
  ${SRC_DIR}/main.cpp
  ${PARSER_SRC}
  ${SCANNER_SRC}
  ${SRC_DIR}/ast/AST.cpp
  ${SRC_DIR}/module/ModuleResolver.cpp
  ${SRC_DIR}/module/Sha256.cpp
  ${SRC_DIR}/type_system/ProductionTypeSystem.cpp
  ${SRC_DIR}/enterprise/EnterpriseLanguage.cpp
  ${SRC_DIR}/visitors/AST_Printer.cpp
  ${SRC_DIR}/visitors/Interpreter.cpp
  ${SRC_DIR}/visitors/IR_Generator.cpp
  ${SRC_DIR}/visitors/IR_Generator_Module.cpp
  ${SRC_DIR}/visitors/SemanticAnalyzer.cpp
  ${SRC_DIR}/visitors/Diagnostics.cpp
  ${SRC_DIR}/evidence/EvidenceEmitter.cpp
  ${SHORTHAND_AI_RUNTIME_SOURCES}
)
add_dependencies(short_hand shorthand_runtime_source_lowering)
target_include_directories(short_hand PRIVATE ${SRC_DIR} ${GENERATED_DIR})
target_compile_options(short_hand PRIVATE ${LLVM_CXXFLAGS})
target_compile_definitions(short_hand PRIVATE SHORTHAND_HAS_ONNXRUNTIME=${SHORTHAND_HAS_ONNXRUNTIME} SHORTHAND_HAS_TENSORRT=${SHORTHAND_HAS_TENSORRT} SHORTHAND_HAS_OPENVINO=${SHORTHAND_HAS_OPENVINO} SHORTHAND_HAS_LIBTORCH=${SHORTHAND_HAS_LIBTORCH} SHORTHAND_HAS_LLAMACPP=${SHORTHAND_HAS_LLAMACPP} SHORTHAND_HAS_OPENBLAS=${SHORTHAND_HAS_OPENBLAS} SHORTHAND_HAS_EIGEN=${SHORTHAND_HAS_EIGEN})
if(SHORTHAND_HAS_ONNXRUNTIME)
  target_include_directories(short_hand PRIVATE ${ONNXRUNTIME_ROOT}/include)
  target_link_directories(short_hand PRIVATE ${ONNXRUNTIME_ROOT}/lib)
  target_link_libraries(short_hand PRIVATE onnxruntime)
  # llvm-config commonly contributes -fno-exceptions. The ONNX Runtime C++ API
  # throws Ort::Exception, and ShortHand deliberately catches it to return a
  # recoverable inference result instead of terminating the process.
  if(MSVC)
    target_compile_options(short_hand PRIVATE /EHsc)
  else()
    target_compile_options(short_hand PRIVATE -fexceptions)
  endif()
endif()
target_link_libraries(short_hand PRIVATE ${LLVM_LDFLAGS})

find_library(FL_LIBRARY fl)
if(FLEX_LIBRARIES)
  target_link_libraries(short_hand PRIVATE ${FLEX_LIBRARIES})
elseif(FL_LIBRARY)
  target_link_libraries(short_hand PRIVATE ${FL_LIBRARY})
endif()

# The language server is a first-class native tool. Keeping it dependency-light
# means every existing CMake platform lane compiles it even when optional AI
# backends are absent. Protocol qualification remains in the dedicated tooling
# gate, while normal install consumers receive the executable in bin/.
add_executable(shorthand_lsp
  ${SRC_DIR}/tooling/LanguageServerMain.cpp
)
target_compile_features(shorthand_lsp PRIVATE cxx_std_17)

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

install(TARGETS
  shorthand_runtime
  shorthand_runtime_shared
  shorthand_ai_bridge
  shorthand_ai_bridge_shared
  shorthand_core
  shorthand_core_shared
  shorthand_serving
  EXPORT ShortHandTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shorthand
)

install(TARGETS shorthand_lsp shorthand_serving_worker
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(TARGET shorthand_prometheus_adapter)
  install(TARGETS shorthand_prometheus_adapter
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

if(TARGET shorthand_otlp_exporter)
  install(TARGETS shorthand_otlp_exporter
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

install(FILES
  ${SRC_DIR}/runtime/ShorthandRuntime.h
  ${SRC_DIR}/runtime/AIRuntimeBridgeAdapter.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shorthand/runtime
)
install(FILES
  ${SRC_DIR}/core/ShorthandCore.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shorthand/core
)
install(FILES
  ${SRC_DIR}/serving/ServingRuntime.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shorthand/serving
)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/abi/shorthand_core_ffi_v1.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shorthand/abi
)
install(FILES
  ${SRC_DIR}/ai_runtime/AI_Types.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shorthand/ai_runtime
)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/abi/shorthand_runtime_abi_v1.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shorthand/abi
)

set(SHORTHAND_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/ShortHand)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ShortHandConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ShortHandConfig.cmake
  INSTALL_DESTINATION ${SHORTHAND_INSTALL_CMAKEDIR}
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/ShortHandConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/shorthand-runtime.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/shorthand-runtime.pc
  @ONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/shorthand-core.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/shorthand-core.pc
  @ONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/shorthand-ai-bridge.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/shorthand-ai-bridge.pc
  @ONLY
)

install(EXPORT ShortHandTargets
  FILE ShortHandTargets.cmake
  NAMESPACE ShortHand::
  DESTINATION ${SHORTHAND_INSTALL_CMAKEDIR}
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/ShortHandConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/ShortHandConfigVersion.cmake
  DESTINATION ${SHORTHAND_INSTALL_CMAKEDIR}
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/shorthand-runtime.pc
  ${CMAKE_CURRENT_BINARY_DIR}/shorthand-ai-bridge.pc
  ${CMAKE_CURRENT_BINARY_DIR}/shorthand-core.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

if(SHORTHAND_BUILD_TESTING)
  enable_testing()

  add_test(
    NAME production_truth_traceability
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_production_truth.sh
  )
  set_tests_properties(production_truth_traceability PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  add_test(
    NAME production_truth_negative
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/governance/test_production_truth_negative.sh
  )
  set_tests_properties(production_truth_negative PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  add_test(
    NAME production_type_memory_model
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_production_type_memory_model.sh
  )
  set_tests_properties(production_type_memory_model PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  add_test(
    NAME functions_control_error_semantics
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_functions_control_error_semantics.sh
  )

  add_test(
    NAME enterprise_packages_stdlib_ffi
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_enterprise_packages_stdlib_ffi.sh
  )
  set_tests_properties(enterprise_packages_stdlib_ffi PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    ENVIRONMENT "SHORTHAND_BIN=$<TARGET_FILE:short_hand>"
  )

  add_test(
    NAME concurrent_serving_runtime
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_concurrent_serving_runtime.sh
  )
  set_tests_properties(concurrent_serving_runtime PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  set_tests_properties(functions_control_error_semantics PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    ENVIRONMENT "SHORTHAND_BIN=$<TARGET_FILE:short_hand>;SHORTHAND_RUNTIME_LIB=$<TARGET_FILE:shorthand_runtime>"
  )

  add_test(
    NAME green_ai_tool_tests
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/green_ai/test_green_ai_tool.sh
  )
  set_tests_properties(green_ai_tool_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  add_test(
    NAME c3eco_language_blocks
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_c3eco_language_blocks.sh
  )
  set_tests_properties(c3eco_language_blocks PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    ENVIRONMENT "SHORTHAND_BIN=$<TARGET_FILE:short_hand>"
  )

  add_test(
    NAME c3eco_certification_profile
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_c3eco_certification_profile.sh
  )
  set_tests_properties(c3eco_certification_profile PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    ENVIRONMENT "SHORTHAND_BIN=$<TARGET_FILE:short_hand>"
  )

  add_test(
    NAME cmake_short_hand_greenai_run
    COMMAND $<TARGET_FILE:short_hand> ${CMAKE_CURRENT_SOURCE_DIR}/Compiler_new_ws/Short_Hand/examples/greenai_report.short run
  )
  set_tests_properties(cmake_short_hand_greenai_run PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PASS_REGULAR_EXPRESSION "GreenAI workload"
  )

  add_test(
    NAME cmake_shorthand_runtime_builds
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:shorthand_runtime>"
  )

  add_test(
    NAME cmake_shorthand_lsp_builds
    COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:shorthand_lsp>"
  )

  add_test(
    NAME runtime_production_packaging
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_runtime_production_packaging.sh
  )
  set_tests_properties(runtime_production_packaging PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  if(TARGET shorthand_prometheus_adapter)
    add_test(
      NAME prometheus_scrape_adapter
      COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_prometheus_scrape_adapter.sh
    )
    set_tests_properties(prometheus_scrape_adapter PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if(TARGET shorthand_otlp_exporter)
    add_test(
      NAME otlp_exporter_adapter
      COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_otlp_exporter_adapter.sh
    )
    set_tests_properties(otlp_exporter_adapter PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
endif()
