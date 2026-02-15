# Test executable
set(TEST_TARGET monkey_test)

add_executable(${TEST_TARGET})

target_sources(
    ${TEST_TARGET}
    PRIVATE
    lexer_test.cpp
)

target_link_libraries(
    ${TEST_TARGET}
    PRIVATE
    monkey_lib
    GTest::gtest_main
)

target_compile_features(${TEST_TARGET} PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})
