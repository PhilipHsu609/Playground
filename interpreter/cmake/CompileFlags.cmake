# Compiler Flags Configuration
# 
# This file defines an interface library 'project_compile_flags' that can be
# linked to targets to apply consistent compiler flags across the project.
# 
# Usage: target_link_libraries(my_target PRIVATE project_compile_flags)

# Define an interface library for compile flags
add_library(project_compile_flags INTERFACE)

# Compiler-specific warnings and flags
target_compile_options(project_compile_flags INTERFACE
    # MSVC flags
    $<$<CXX_COMPILER_ID:MSVC>:/W4>           # Warning level 4
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->  # Standards conformance
    
    # GCC/Clang flags
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wextra>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wshadow>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wnon-virtual-dtor>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wold-style-cast>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wcast-align>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Woverloaded-virtual>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wconversion>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wsign-conversion>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wmisleading-indentation>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wnull-dereference>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-pedantic>
    
    # Debug/Release flags for GCC/Clang
    $<$<AND:$<CONFIG:DEBUG>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>>:-g>
    $<$<AND:$<CONFIG:RELEASE>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>>:-O3>
)

# Platform-specific additional flags for GCC/Clang
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(project_compile_flags INTERFACE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror>  # Treat warnings as errors on Linux
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_options(project_compile_flags INTERFACE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wshorten-64-to-32>  # Warn on 64-bit to 32-bit conversions (macOS)
    )
endif()

