cmake_minimum_required(VERSION 3.15)

# Include CMake utilities
include(cmake/debug.cmake)
include(cmake/CompileFlags.cmake)

# Project definition
project(
    tinyrenderer
    VERSION 1.0
    DESCRIPTION "Software renderer following ssloy/tinyrenderer"
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
    GIT_TAG 12.1.0
    SYSTEM
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    SYSTEM
)

# stb is header-only with no CMake project; fetch and expose the include dir
# through an INTERFACE library so targets can link `stb_image` to pull it in.
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
    SYSTEM
)

FetchContent_MakeAvailable(fmt googletest stb)

add_library(stb_image INTERFACE)
target_include_directories(stb_image SYSTEM INTERFACE ${stb_SOURCE_DIR})

# Add source directory
add_subdirectory(src)

# Enable testing and add test directory
enable_testing()
add_subdirectory(test)
