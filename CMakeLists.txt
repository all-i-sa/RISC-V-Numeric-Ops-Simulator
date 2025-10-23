cmake_minimum_required(VERSION 3.20)
project(RISC_V_Simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core library
add_library(core_objs
        src/core/bitvec.cpp
        src/core/twos.cpp
)
target_include_directories(core_objs PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_options(core_objs PRIVATE -Wall -Wextra -Wpedantic)

# CLI binary (current main.cpp)
add_executable(RISC_V_Simulator main.cpp)
target_link_libraries(RISC_V_Simulator PRIVATE core_objs)

# Tests (GoogleTest)
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(core_tests
        tests/bitvec_tests.cpp
        tests/twos_tests.cpp
)
target_link_libraries(core_tests PRIVATE core_objs GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(core_tests)
