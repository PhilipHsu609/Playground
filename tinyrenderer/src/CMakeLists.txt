add_executable(
    main
    main.cpp
    TGAImage.cpp
    Drawer.cpp
    Vector.cpp
    Model.cpp
)

target_include_directories(
    main
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    main
    PRIVATE
    compile_flags_interface
    fmt::fmt
)
