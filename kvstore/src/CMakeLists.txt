# Storage-engine library (all components except main)
add_library(kvstore_lib STATIC)

target_sources(
    kvstore_lib
    PRIVATE
    version.cpp
)

target_include_directories(
    kvstore_lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    kvstore_lib
    PUBLIC
    project_compile_flags
)

target_compile_features(kvstore_lib PUBLIC cxx_std_23)

# CLI / main executable
add_executable(kvstore main.cpp)

target_link_libraries(
    kvstore
    PRIVATE
    kvstore_lib
)
