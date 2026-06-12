cmake_minimum_required(VERSION 3.15)

# Project definition
project(
    kvstore
    VERSION 0.1.0
    DESCRIPTION "From-scratch LSM-tree key-value storage engine"
    LANGUAGES CXX
)

# C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use libc++ under Clang on Linux. Set before FetchContent so the fetched deps
# build with the same stdlib (avoids a libc++/libstdc++ ABI clash at link time).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()

# Included after project() so the compiler/system vars the flag guards read are set.
include(cmake/debug.cmake)
include(cmake/CompileFlags.cmake)

# Export compile commands for clangd, clang-tidy, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies (via FetchContent)
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    SYSTEM
)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
    SYSTEM
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest benchmark)

# Add source directory
add_subdirectory(src)

# Enable testing and add test directory
enable_testing()
add_subdirectory(test)

# Benchmarks
add_subdirectory(bench)
