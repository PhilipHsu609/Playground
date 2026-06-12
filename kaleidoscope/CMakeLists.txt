cmake_minimum_required(VERSION 3.15)

# Project definition
project(
    kaleidoscope
    VERSION 0.1.0
    DESCRIPTION "From-scratch LLVM Kaleidoscope: lexer, parser, IR codegen, JIT"
    LANGUAGES C CXX  # C needed: LLVM's CMake config runs C source-compile checks
)

# C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Included after project() so the compiler/system vars the flag guards read are set.
include(cmake/debug.cmake)
include(cmake/CompileFlags.cmake)

# Export compile commands for clangd, clang-tidy, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LLVM via its CMake package; LLVM_DIR (set by the Makefile) selects the install.
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")

# GoogleTest (via FetchContent)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    SYSTEM
)
FetchContent_MakeAvailable(googletest)

# Add source directory
add_subdirectory(src)

# Enable testing and add test directory
enable_testing()
add_subdirectory(test)
