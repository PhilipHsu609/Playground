# Test executable
set(TEST_TARGET tinyrenderer_test)

add_executable(${TEST_TARGET})

target_sources(
    ${TEST_TARGET}
    PRIVATE
    VectorTest.cpp
)

target_link_libraries(
    ${TEST_TARGET}
    PRIVATE
    tinyrenderer_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})
