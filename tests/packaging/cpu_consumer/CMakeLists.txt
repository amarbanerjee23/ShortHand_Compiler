cmake_minimum_required(VERSION 3.21)
project(InstalledCpuInference LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(ShortHand 1 CONFIG REQUIRED)
if(NOT ShortHand_ONNXRUNTIME_ENABLED)
  message(FATAL_ERROR "Installed CPU package must enable ONNX Runtime")
endif()
enable_testing()
foreach(kind static shared)
  add_executable(cpu_${kind} cpu_consumer.cpp)
  if(kind STREQUAL "static")
    target_link_libraries(cpu_${kind} PRIVATE ShortHand::runtime)
  else()
    target_link_libraries(cpu_${kind} PRIVATE ShortHand::runtime_shared)
  endif()
  add_test(NAME cpu_${kind} COMMAND cpu_${kind} "${MODEL_FILE}")
endforeach()
# Compile a ShortHand source with the installed compiler, then link its bitcode
# against the installed CMake target (including transitive ONNX dependencies).
find_program(SHORTHAND_COMPILER NAMES short_hand REQUIRED HINTS "${PACKAGE_BIN}" NO_DEFAULT_PATH)
find_program(SHORTHAND_LLC NAMES llc-18 llc REQUIRED)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cpu_source.o"
  COMMAND "${SHORTHAND_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/cpu_source.short" compile-bc
  COMMAND "${SHORTHAND_LLC}" -filetype=obj -relocation-model=pic cpu_source.bc -o cpu_source.o
  DEPENDS cpu_source.short VERBATIM)
add_executable(cpu_source "${CMAKE_CURRENT_BINARY_DIR}/cpu_source.o")
set_target_properties(cpu_source PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cpu_source PRIVATE ShortHand::runtime)
add_test(NAME cpu_source COMMAND cpu_source)
set_tests_properties(cpu_source PROPERTIES
  WORKING_DIRECTORY "${MODEL_DIRECTORY}" PASS_REGULAR_EXPRESSION "Installed compiled program: 42")
if(WIN32)
  # Windows searches the system directory before PATH. Stage the imported DLLs
  # next to the consumers so a preinstalled older ONNX cannot shadow the bundle.
  # These three executables share one output directory; copy once after linking.
  add_custom_target(stage_cpu_runtime_dlls ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "$<TARGET_RUNTIME_DLLS:cpu_shared>" "$<TARGET_FILE_DIR:cpu_shared>"
    DEPENDS cpu_static cpu_shared cpu_source
    COMMAND_EXPAND_LISTS VERBATIM)
endif()
