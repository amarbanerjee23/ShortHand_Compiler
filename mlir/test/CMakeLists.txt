find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(SHORTHAND_FILECHECK NAMES FileCheck HINTS "${LLVM_TOOLS_BINARY_DIR}" REQUIRED NO_DEFAULT_PATH)
find_program(SHORTHAND_LIT NAMES llvm-lit lit HINTS "${LLVM_TOOLS_BINARY_DIR}")
if(NOT SHORTHAND_LIT)
  find_file(SHORTHAND_LIT_PY NAMES lit.py HINTS "${LLVM_TOOLS_BINARY_DIR}/../build/utils/lit" REQUIRED)
  set(SHORTHAND_LIT "${SHORTHAND_LIT_PY}")
endif()
set(SHORTHAND_MLIR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
configure_file(lit.site.cfg.py.in lit.site.cfg.py @ONLY)
add_test(NAME mlir_lit COMMAND "${Python3_EXECUTABLE}" "${SHORTHAND_LIT}"
  -sv "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME mlir_generation COMMAND "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_generation.py"
  "${SHORTHAND_MLIR_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/.." "${MLIR_DIR}")
add_test(NAME mlir_installed_consumer COMMAND "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_install.py"
  "${SHORTHAND_MLIR_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/.."
  "${MLIR_DIR}" "${CMAKE_CXX_COMPILER}" "${SHORTHAND_MLIR_SANITIZERS}")
set_tests_properties(mlir_lit mlir_generation mlir_installed_consumer PROPERTIES TIMEOUT 300)
add_custom_target(check-shorthand-mlir
  COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
  DEPENDS shorthand-opt)
