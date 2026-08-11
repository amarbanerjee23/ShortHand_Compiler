cmake_minimum_required(VERSION 3.16)
project(ShortHandCTestParity NONE)

enable_testing()

if(NOT DEFINED SHORTHAND_SOURCE_DIR)
  get_filename_component(SHORTHAND_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

find_program(SHORTHAND_GNU_MAKE NAMES make gmake REQUIRED)
set(SHORTHAND_MAKE_DIR "${SHORTHAND_SOURCE_DIR}/Compiler_new_ws/Short_Hand/src")
set(SHORTHAND_EXPECTED_FILE "${CMAKE_CURRENT_LIST_DIR}/expected_make_targets.txt")

file(STRINGS "${SHORTHAND_EXPECTED_FILE}" SHORTHAND_MAKE_TARGETS)
foreach(target IN LISTS SHORTHAND_MAKE_TARGETS)
  if(target STREQUAL "")
    continue()
  endif()
  string(REPLACE "-" "_" test_suffix "${target}")
  add_test(
    NAME "make_${test_suffix}"
    COMMAND "${SHORTHAND_GNU_MAKE}" -C "${SHORTHAND_MAKE_DIR}" "${target}"
  )
  set_tests_properties("make_${test_suffix}" PROPERTIES
    RUN_SERIAL TRUE
    TIMEOUT 900
    LABELS "make-parity"
    ENVIRONMENT "LC_ALL=C;TZ=UTC"
  )
endforeach()
