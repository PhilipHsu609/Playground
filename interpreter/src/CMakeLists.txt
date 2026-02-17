# Interpreter library (all components except main)
add_library(monkey_lib STATIC)

target_sources(
    monkey_lib
    PRIVATE
    ast.cpp
    lexer.cpp
    parser.cpp
    repl.cpp
)

target_include_directories(
    monkey_lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    monkey_lib
    PUBLIC
    project_compile_flags
    fmt::fmt
    magic_enum::magic_enum
)

target_compile_features(monkey_lib PUBLIC cxx_std_23)

# REPL / main executable
add_executable(monkey main.cpp)

target_link_libraries(
    monkey
    PRIVATE
    monkey_lib
)
