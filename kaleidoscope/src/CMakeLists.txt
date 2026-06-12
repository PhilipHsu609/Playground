# Compiler library (all components except main)
add_library(kaleidoscope_lib STATIC)

target_sources(
    kaleidoscope_lib
    PRIVATE
    version.cpp
)

# LLVM is a private implementation detail: PRIVATE so its headers don't leak into
# consumers. Link the dynamic libLLVM, not the static component libs.
target_include_directories(
    kaleidoscope_lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    SYSTEM PRIVATE
    ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(
    kaleidoscope_lib
    PUBLIC
    project_compile_flags
    PRIVATE
    LLVM
)

target_compile_features(kaleidoscope_lib PUBLIC cxx_std_23)

# REPL / main executable
add_executable(kaleidoscope main.cpp)

target_link_libraries(
    kaleidoscope
    PRIVATE
    kaleidoscope_lib
)
