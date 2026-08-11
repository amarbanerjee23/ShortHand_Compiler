cmake_minimum_required(VERSION 3.16)
project(ShortHandInstalledConsumer LANGUAGES CXX)

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
