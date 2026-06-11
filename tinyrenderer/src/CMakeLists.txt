# Renderer library (all components except main)
add_library(tinyrenderer_lib STATIC)

target_sources(
    tinyrenderer_lib
    PRIVATE
    TGAImage.cpp
    Drawer.cpp
    Model.cpp
    Shader.cpp
    AmbientOcclusion.cpp
    Outline.cpp
)

target_include_directories(
    tinyrenderer_lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    tinyrenderer_lib
    PUBLIC
    project_compile_flags
    fmt::fmt
    PRIVATE
    stb_image
)

# Main executable
add_executable(tinyrenderer main.cpp)

target_link_libraries(
    tinyrenderer
    PRIVATE
    tinyrenderer_lib
)
