# Test executable
set(TEST_TARGET kaleidoscope_test)

add_executable(${TEST_TARGET})

target_sources(
    ${TEST_TARGET}
    PRIVATE
    version_test.cpp
)

target_link_libraries(
    ${TEST_TARGET}
    PRIVATE
    kaleidoscope_lib
    GTest::gtest_main
)

target_compile_features(${TEST_TARGET} PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
