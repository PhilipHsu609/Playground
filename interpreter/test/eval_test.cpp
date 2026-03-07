#include "monkey/env.h"
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
    auto env = makeEnvironment();
    return eval(*program, env);
}

void testIntegerObject(const Object &obj, int64_t expected) {
    ASSERT_TRUE(std::holds_alternative<int64_t>(obj));
    ASSERT_EQ(std::get<int64_t>(obj), expected);
}

void testBooleanObject(const Object &obj, bool expected) {
    ASSERT_TRUE(std::holds_alternative<bool>(obj));
    ASSERT_EQ(std::get<bool>(obj), expected);
}

TEST(EvalTest, IntegerExpression) {
    std::vector<std::pair<std::string, int64_t>> tests = {
        {"5", 5},
        {"10", 10},
        {"-5", -5},
        {"-10", -10},
        {"5 + 5 + 5 + 5 - 10", 10},
        {"2 * 2 * 2 * 2 * 2", 32},
        {"-50 + 100 + -50", 0},
        {"5 * 2 + 10", 20},
        {"5 + 2 * 10", 25},
        {"20 + 2 * -10", 0},
        {"50 / 2 * 2 + 10", 60},
        {"2 * (5 + 10)", 30},
        {"3 * 3 * 3 + 10", 37},
        {"3 * (3 * 3) + 10", 37},
        {"(5 + 10 * 2 + 15 /3) * 2 + -10", 50}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        testIntegerObject(evaluated, expected);
    }
}

TEST(EvalTest, BooleanExpression) {
    std::vector<std::pair<std::string, bool>> tests = {{"true", true},
                                                       {"false", false},
                                                       {"1 < 2", true},
                                                       {"1 > 2", false},
                                                       {"1 < 1", false},
                                                       {"1 > 1", false},
                                                       {"1 == 1", true},
                                                       {"1 != 1", false},
                                                       {"1 == 2", false},
                                                       {"1 != 2", true},
                                                       {"(1 < 2) == true", true},
                                                       {"(1 < 2) == false", false},
                                                       {"(1 > 2) == true", false},
                                                       {"(1 > 2) == false", true}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        testBooleanObject(evaluated, expected);
    }
}

TEST(EvalTest, BangOperator) {
    std::vector<std::pair<std::string, bool>> tests = {
        {"!true", false}, {"!false", true},   {"!5", false},
        {"!!true", true}, {"!!false", false}, {"!!5", true}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        testBooleanObject(evaluated, expected);
    }
}

TEST(EvalTest, IfElseExpression) {
    std::vector<std::pair<std::string, Object>> tests = {
        {"if (true) { 10 }", 10},
        {"if (false) { 10 }", nullptr},
        {"if (1) { 10 }", 10},
        {"if (1 < 2) { 10 }", 10},
        {"if (1 > 2) { 10 }", nullptr},
        {"if (1 > 2) { 10 } else { 20 }", 20},
        {"if (1 < 2) { 10 } else { 20 }", 10}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        if (std::holds_alternative<int64_t>(expected)) {
            testIntegerObject(evaluated, std::get<int64_t>(expected));
        } else {
            ASSERT_TRUE(std::holds_alternative<std::nullptr_t>(evaluated));
        }
    }
}

TEST(EvalTest, ReturnStatements) {
    std::vector<std::pair<std::string, int64_t>> tests = {
        {"return 10;", 10},
        {"return 10; 9;", 10},
        {"return 2 * 5; 9;", 10},
        {"9; return 2 * 5; 9;", 10},
        {"if (10 > 1) { if (10 > 1) { return 10; } return 1; }", 10}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        testIntegerObject(evaluated, expected);
    }
}

TEST(EvalTest, ErrorHandling) {
    std::vector<std::pair<std::string, std::string>> tests = {
        {"5 + true;", "type mismatch: 5 + true"},
        {"5 + true; 5;", "type mismatch: 5 + true"},
        {"-true", "unknown operator: -true"},
        {"true + false;", "unknown operator: true + false"},
        {"5; true + false; 5", "unknown operator: true + false"},
        {"if (10 > 1) { true + false; }", "unknown operator: true + false"},
        {"foobar", "identifier not found: foobar"}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        ASSERT_TRUE(std::holds_alternative<Error>(evaluated));
        ASSERT_EQ(std::get<Error>(evaluated), expected);
    }
}

TEST(EvalTest, LetStatements) {
    std::vector<std::pair<std::string, int64_t>> tests = {
        {"let a = 5; a;", 5},
        {"let a = 5 * 5; a;", 25},
        {"let a = 5; let b = a; b;", 5},
        {"let a = 5; let b = a; let c = a + b + 5; c;", 15}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        testIntegerObject(evaluated, expected);
    }
}

TEST(EvalTest, FunctionObject) {
    std::string input = "fn(x) { x + 2; };";

    Object evaluated = testEval(input);
    ASSERT_TRUE(std::holds_alternative<Box<Function>>(evaluated));

    auto fn = std::get<Box<Function>>(evaluated);
    ASSERT_EQ(fn->parameters.size(), 1);
    ASSERT_EQ(tokenLiteral(fn->parameters[0]), "x");

    std::string expectedBody = "{ (x + 2) }";
    ASSERT_EQ(toString(fn->body), expectedBody);
}

TEST(EvalTest, FunctionApplication) {
    std::vector<std::pair<std::string, int64_t>> tests = {
        {"let identity = fn(x) { x; }; identity(5);", 5},
        {"let identity = fn(x) { return x; }; identity(5);", 5},
        {"let double = fn(x) { x * 2; }; double(5);", 10},
        {"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
        {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
        {"fn(x) { x; }(5)", 5}};
    for (const auto &[input, expected] : tests) {
        Object evaluated = testEval(input);
        testIntegerObject(evaluated, expected);
    }
}
