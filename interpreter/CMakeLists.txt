cmake_minimum_required(VERSION 3.15)

# Include CMake utilities
include(cmake/debug.cmake)
include(cmake/CompileFlags.cmake)

# Project definition
project(
    monkey
    VERSION 0.1.0
    DESCRIPTION "Monkey programming language interpreter"
    LANGUAGES CXX
)

# C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd, clang-tidy, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies (via FetchContent)
include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.1.4
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)

FetchContent_MakeAvailable(fmt googletest)

# Add source directory
add_subdirectory(src)

# Enable testing and add test directory
enable_testing()
add_subdirectory(test)
