# Compile All Files Helper
#
# This CMake module automatically creates an executable for each .cpp file
# in the current directory. Useful for examples or small utilities.
#
# Usage: include(cmake/compile_all_files.cmake) in a CMakeLists.txt
#
# Note: Not typically used in the main project, but useful for examples/
#       or utils/ directories where you want one executable per file.

# Set output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Find all .cpp files in the current directory
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

# Create an executable for each source file
foreach(SOURCE_FILE ${SOURCE_FILES})
    get_filename_component(EXECUTABLE_NAME ${SOURCE_FILE} NAME_WE)
    add_executable(${EXECUTABLE_NAME})
    target_sources(${EXECUTABLE_NAME} PRIVATE ${SOURCE_FILE})
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE fmt::fmt)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE project_compile_flags)
endforeach()
