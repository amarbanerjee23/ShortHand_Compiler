cmake_minimum_required(VERSION 3.20)
project(ShortHandMLIR VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MLIR REQUIRED CONFIG)
if(NOT LLVM_VERSION_MAJOR EQUAL 18)
  message(FATAL_ERROR "ShortHand MLIR v1 requires LLVM/MLIR 18.x")
endif()
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}" "${LLVM_CMAKE_DIR}")
include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(SHORTHAND_MLIR_TESTING "Register mandatory MLIR qualification tests" ON)
option(SHORTHAND_MLIR_SANITIZERS "Instrument ShortHand dialect and consumers with ASan/UBSan" OFF)
if(SHORTHAND_MLIR_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include")
add_definitions(${LLVM_DEFINITIONS})
add_subdirectory(include/ShortHand/IR)

add_library(ShortHandMLIRDialect STATIC lib/ShortHandDialect.cpp)
add_library(ShortHandMLIR::Dialect ALIAS ShortHandMLIRDialect)
add_dependencies(ShortHandMLIRDialect ShortHandMLIRIncGen)
target_compile_features(ShortHandMLIRDialect PUBLIC cxx_std_17)
target_include_directories(ShortHandMLIRDialect PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(ShortHandMLIRDialect SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
target_link_libraries(ShortHandMLIRDialect PUBLIC MLIRIR MLIRSupport MLIRSideEffectInterfaces MLIRLLVMDialect)
set_target_properties(ShortHandMLIRDialect PROPERTIES EXPORT_NAME Dialect POSITION_INDEPENDENT_CODE ON)

add_library(ShortHandMLIRLowering STATIC lib/SemanticVerifier.cpp lib/SemanticLowering.cpp lib/LowerToLLVM.cpp)
add_library(ShortHandMLIR::Lowering ALIAS ShortHandMLIRLowering)
target_link_libraries(ShortHandMLIRLowering PUBLIC ShortHandMLIRDialect
  MLIRLLVMDialect MLIRFuncDialect MLIRLLVMCommonConversion MLIRFuncToLLVM
  MLIRArithToLLVM MLIRControlFlowToLLVM MLIRTransforms MLIRPass
  MLIRTargetLLVMIRExport MLIRLLVMToLLVMIRTranslation MLIRBuiltinToLLVMIRTranslation)
set_target_properties(ShortHandMLIRLowering PROPERTIES EXPORT_NAME Lowering POSITION_INDEPENDENT_CODE ON)

add_executable(shorthand-opt tools/shorthand-opt.cpp)
target_link_libraries(shorthand-opt PRIVATE ShortHandMLIRLowering MLIROptLib MLIRFuncDialect MLIRTransforms)

install(TARGETS ShortHandMLIRDialect ShortHandMLIRLowering shorthand-opt EXPORT ShortHandMLIRTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ShortHand DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h" PATTERN "*.td")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/ShortHand" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h.inc")
install(FILES README.md LICENSE-LLVM.txt DESTINATION ${CMAKE_INSTALL_DATADIR}/shorthand/mlir)
install(FILES ../docs/mlir_lowering.md DESTINATION ${CMAKE_INSTALL_DATADIR}/shorthand/docs)
install(DIRECTORY examples/ DESTINATION ${CMAKE_INSTALL_DATADIR}/shorthand/mlir/examples)
set(package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/ShortHandMLIR")
configure_package_config_file(cmake/ShortHandMLIRConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/ShortHandMLIRConfig.cmake" INSTALL_DESTINATION "${package_dir}")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/ShortHandMLIRConfigVersion.cmake"
  VERSION 1.0.0 COMPATIBILITY SameMajorVersion)
install(EXPORT ShortHandMLIRTargets NAMESPACE ShortHandMLIR:: DESTINATION "${package_dir}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ShortHandMLIRConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/ShortHandMLIRConfigVersion.cmake" DESTINATION "${package_dir}")

if(SHORTHAND_MLIR_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
