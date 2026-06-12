# Debug Utility Function
#
# This function prints useful CMake variables for debugging build issues.
# Usage: Call debug() in CMakeLists.txt to print variable values.
# Example: debug(MY_CUSTOM_VAR ANOTHER_VAR)

function(debug)
    include(CMakePrintHelpers)

    # Project variables (defined by project())
    cmake_print_variables(PROJECT_NAME)
    cmake_print_variables(PROJECT_SOURCE_DIR) # Source directory
    cmake_print_variables(PROJECT_BINARY_DIR) # Build directory

    # CMake variables (defined by CMakeLists.txt)
    cmake_print_variables(CMAKE_SOURCE_DIR) # Top level source directory (prefer PROJECT_SOURCE_DIR)
    cmake_print_variables(CMAKE_BINARY_DIR) # Top level build directory (prefer PROJECT_BINARY_DIR)

    cmake_print_variables(CMAKE_CURRENT_LIST_DIR) # Current source directory
    cmake_print_variables(CMAKE_CURRENT_LIST_FILE) # Current source file

    cmake_print_variables(CMAKE_CURRENT_SOURCE_DIR) # Current source directory (may change with include())
    cmake_print_variables(CMAKE_CURRENT_BINARY_DIR) # Current build directory (may change with include())

    cmake_print_variables(CMAKE_RUNTIME_OUTPUT_DIRECTORY) # Output directory for executables

    # Additional arguments - print user-specified variables
    foreach(var ${ARGN})
        if(DEFINED ${var})
            cmake_print_variables(${var})
        else()
            message(STATUS "${var} is not defined")
        endif()
    endforeach()
endfunction()

