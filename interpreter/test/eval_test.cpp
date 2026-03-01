#include "monkey/eval.h"
#include "monkey/lexer.h"
#include "monkey/object.h"
#include "monkey/parser.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace monkey;

Object testEval(const std::string &input) {
    auto parser = Parser(Lexer(input));
    auto program = parser.parseProgram();
    return eval(*program);
}

void testIntegerObject(const Object &obj, int64_t expected) {
    ASSERT_TRUE(std::holds_alternative<int64_t>(obj));
    ASSERT_EQ(std::get<int64_t>(obj), expected)
        << "object has wrong value. got=" << std::get<int64_t>(obj)
        << ", want=" << expected;
}

void testBooleanObject(const Object &obj, bool expected) {
    ASSERT_TRUE(std::holds_alternative<bool>(obj));
    ASSERT_EQ(std::get<bool>(obj), expected)
        << "object has wrong value. got=" << std::get<bool>(obj) << ", want=" << expected;
}

TEST(EvalTest, IntegerExpression) {
    std::vector<std::pair<std::string, int64_t>> tests = {{"5", 5}, {"10", 10}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        testIntegerObject(evaluated, expected);
    }
}

TEST(EvalTest, BooleanExpression) {
    std::vector<std::pair<std::string, bool>> tests = {{"true", true}, {"false", false}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        testBooleanObject(evaluated, expected);
    }
}