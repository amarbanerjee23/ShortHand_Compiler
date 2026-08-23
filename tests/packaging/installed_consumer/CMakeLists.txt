cmake_minimum_required(VERSION 3.16)
project(ShortHandInstalledConsumer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ShortHand 1 CONFIG REQUIRED)

enable_testing()

add_executable(runtime_static runtime_consumer.cpp)
target_link_libraries(runtime_static PRIVATE ShortHand::runtime)
add_test(NAME installed_runtime_static COMMAND runtime_static)

add_executable(runtime_shared runtime_consumer.cpp)
target_link_libraries(runtime_shared PRIVATE ShortHand::runtime_shared)
add_test(NAME installed_runtime_shared COMMAND runtime_shared)

add_executable(abi_v1_static abi_v1_consumer.cpp)
target_link_libraries(abi_v1_static PRIVATE ShortHand::runtime)
add_test(NAME installed_abi_v1_static COMMAND abi_v1_static)

add_executable(abi_v1_shared abi_v1_consumer.cpp)
target_link_libraries(abi_v1_shared PRIVATE ShortHand::runtime_shared)
add_test(NAME installed_abi_v1_shared COMMAND abi_v1_shared)

add_executable(bridge_static bridge_consumer.cpp)
target_link_libraries(bridge_static PRIVATE ShortHand::ai_bridge)
add_test(NAME installed_bridge_static COMMAND bridge_static)

add_executable(bridge_shared bridge_consumer.cpp)
target_link_libraries(bridge_shared PRIVATE ShortHand::ai_bridge_shared)
add_test(NAME installed_bridge_shared COMMAND bridge_shared)

add_executable(core_c_static core_c_consumer.c)
set_target_properties(core_c_static PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(core_c_static PRIVATE ShortHand::core)
add_test(NAME installed_core_c_static COMMAND core_c_static)

add_executable(core_c_shared core_c_consumer.c)
set_target_properties(core_c_shared PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(core_c_shared PRIVATE ShortHand::core_shared)
add_test(NAME installed_core_c_shared COMMAND core_c_shared)

add_executable(core_cpp_static core_cpp_consumer.cpp)
target_link_libraries(core_cpp_static PRIVATE ShortHand::core)
add_test(NAME installed_core_cpp_static COMMAND core_cpp_static)

add_executable(core_cpp_shared core_cpp_consumer.cpp)
target_link_libraries(core_cpp_shared PRIVATE ShortHand::core_shared)
add_test(NAME installed_core_cpp_shared COMMAND core_cpp_shared)
