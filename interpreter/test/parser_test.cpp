#include "monkey/ast.h"
#include "monkey/lexer.h"
#include "monkey/parser.h"

#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <ios>
#include <ranges>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

using namespace monkey;

template <typename... T>
// NOLINTNEXTLINE(readability-identifier-naming) - type_trait naming convention
constexpr bool always_false = false;

void checkParserErrors(const Parser &parser) {
    const auto &errors = parser.errors();
    if (errors.empty()) {
        return;
    }
    FAIL() << "parser has " << errors.size() << " errors:\n"
           << fmt::format("{}", fmt::join(errors, "\n"));
}

using Literal = std::variant<int64_t, bool, std::string>;

void testIntegerLiteral(const Expression &expr, int64_t value) {
    const auto *intLiteral = std::get_if<IntegerLiteral>(&expr);

    if (intLiteral == nullptr) {
        FAIL() << "expression not IntegerLiteral. got=" << typeid(expr).name();
    }

    EXPECT_EQ(tokenLiteral(*intLiteral), std::to_string(value))
        << "intLiteral.tokenLiteral() not '" << value
        << "'. got=" << tokenLiteral(*intLiteral);
    EXPECT_EQ(intLiteral->value, value)
        << "intLiteral.value not " << value << ". got=" << intLiteral->value;
}

void testBooleanLiteral(const Expression &expr, bool value) {
    const auto *boolLiteral = std::get_if<BooleanLiteral>(&expr);

    if (boolLiteral == nullptr) {
        FAIL() << "expression not BooleanLiteral. got=" << typeid(expr).name();
    }

    EXPECT_EQ(tokenLiteral(*boolLiteral), value ? "true" : "false")
        << "boolLiteral.tokenLiteral() not '" << (value ? "true" : "false")
        << "'. got=" << tokenLiteral(*boolLiteral);
    EXPECT_EQ(boolLiteral->value, value)
        << "boolLiteral.value not " << std::boolalpha << value
        << ". got=" << std::boolalpha << boolLiteral->value;
}

void testIdentifier(const Expression &expr, const std::string &value) {
    const auto *ident = std::get_if<Identifier>(&expr);

    if (ident == nullptr) {
        FAIL() << "expression not Identifier. got=" << typeid(expr).name();
    }

    EXPECT_EQ(tokenLiteral(*ident), value)
        << "ident.tokenLiteral() not '" << value << "'. got=" << tokenLiteral(*ident);
}

void testLiteralExpression(const Expression &expr, auto &&expected) {
    using T = std::decay_t<decltype(expected)>;
    if constexpr (std::is_same_v<T, int64_t>) {
        testIntegerLiteral(expr, expected);
    } else if constexpr (std::is_same_v<T, bool>) {
        testBooleanLiteral(expr, expected);
    } else if constexpr (std::is_integral_v<T>) {
        testIntegerLiteral(expr, static_cast<int64_t>(expected));
    } else if constexpr (std::is_convertible_v<T, std::string>) {
        testIdentifier(expr, expected);
    } else if constexpr (std::is_same_v<T, Literal>) {
        std::visit([&](const auto &literal) { testLiteralExpression(expr, literal); },
                   expected);
    } else {
        static_assert(always_false<T>, "unsupported literal type");
    }
}

void testInfixExpression(const Expression &expr, const Literal &leftValue,
                         const std::string &op, const Literal &rightValue) {
    const auto *infixExpr = std::get_if<Box<InfixExpression>>(&expr);

    if (infixExpr == nullptr) {
        FAIL() << "expression not InfixExpression. got=" << typeid(expr).name();
    }

    testLiteralExpression((*infixExpr)->left, leftValue);

    EXPECT_EQ((*infixExpr)->op, op)
        << "operator is not '" << op << "'. got=" << (*infixExpr)->op;

    testLiteralExpression((*infixExpr)->right, rightValue);
}

TEST(ParserTest, LetStatements) {
    std::string input = R"(
let x = 5;
let y = 10;
let foobar = 838383;
)";

    auto parser = Parser(std::make_unique<Lexer>(input));
    auto program = parser.parseProgram();

    checkParserErrors(parser);

    if (program == nullptr) {
        FAIL() << "parseProgram() returned nullptr";
    }

    if (program->statements.size() != 3) {
        FAIL() << "program.statements does not contain 3 statements. got="
               << program->statements.size();
    }

    std::vector<std::string> expectedIdentifiers{"x", "y", "foobar"};
    // std::vector<std::string> expected_values{"5", "10", "838383"};

    for (const auto &[i, stmt] : std::views::enumerate(program->statements)) {
        auto idx = static_cast<size_t>(i);
        const auto *letStmt = std::get_if<LetStatement>(&stmt);

        if (letStmt == nullptr) {
            FAIL() << "stmt not LetStatement. got=" << typeid(stmt).name();
        }

        EXPECT_EQ(tokenLiteral(*letStmt), "let")
            << "let_stmt.tokenLiteral() not 'let'. got=" << tokenLiteral(*letStmt);

        testIdentifier(letStmt->name, expectedIdentifiers[idx]);

        // EXPECT_EQ(tokenLiteral(let_stmt->value), expected_values[idx])
        //     << "let_stmt.value not '" << expected_values[idx]
        //     << "'. got=" << tokenLiteral(let_stmt->value);
    }
}

TEST(ParserTest, ReturnStatements) {
    std::string input = R"(
return 5;
return 10;
return 993322;
)";

    auto parser = Parser(std::make_unique<Lexer>(input));
    auto program = parser.parseProgram();

    checkParserErrors(parser);

    if (program == nullptr) {
        FAIL() << "parseProgram() returned nullptr";
    }

    if (program->statements.size() != 3) {
        FAIL() << "program.statements does not contain 3 statements. got="
               << program->statements.size();
    }

    // std::vector<std::string> expected_values{"5", "10", "993322"};

    for (const auto &[i, stmt] : std::views::enumerate(program->statements)) {
        // auto idx = static_cast<size_t>(i);
        const auto *returnStmt = std::get_if<ReturnStatement>(&stmt);

        if (returnStmt == nullptr) {
            FAIL() << "stmt not ReturnStatement. got=" << typeid(stmt).name();
        }

        EXPECT_EQ(tokenLiteral(*returnStmt), "return")
            << "return_stmt.tokenLiteral() not 'return'. got="
            << tokenLiteral(*returnStmt);
        // EXPECT_EQ(tokenLiteral(returnStmt->value), expected_values[idx])
        //     << "return_stmt.value not '" << expected_values[idx]
        //     << "'. got=" << tokenLiteral(returnStmt->value);
    }
}

TEST(ParserTest, IdentifierExpression) {
    std::string input = "foobar;";

    auto parser = Parser(std::make_unique<Lexer>(input));
    auto program = parser.parseProgram();

    checkParserErrors(parser);

    if (program == nullptr) {
        FAIL() << "parseProgram() returned nullptr";
    }

    if (program->statements.size() != 1) {
        FAIL() << "program.statements does not contain 1 statement. got="
               << program->statements.size();
    }

    const auto &stmt = program->statements[0];
    const auto *exprStmt = std::get_if<ExpressionStatement>(&stmt);

    if (exprStmt == nullptr) {
        FAIL() << "stmt not ExpressionStatement. got=" << typeid(stmt).name();
    }

    testIdentifier(exprStmt->expression, "foobar");
}

TEST(ParserTest, IntegerLiteralExpression) {
    std::string input = "5;";

    auto parser = Parser(std::make_unique<Lexer>(input));
    auto program = parser.parseProgram();

    checkParserErrors(parser);

    if (program == nullptr) {
        FAIL() << "parseProgram() returned nullptr";
    }

    if (program->statements.size() != 1) {
        FAIL() << "program.statements does not contain 1 statement. got="
               << program->statements.size();
    }

    const auto &stmt = program->statements[0];
    const auto *exprStmt = std::get_if<ExpressionStatement>(&stmt);

    if (exprStmt == nullptr) {
        FAIL() << "stmt not ExpressionStatement. got=" << typeid(stmt).name();
    }

    testIntegerLiteral(exprStmt->expression, 5);
}

TEST(ParserTest, BooleanLiteralExpression) {
    std::string input = "true;";

    auto parser = Parser(std::make_unique<Lexer>(input));
    auto program = parser.parseProgram();

    checkParserErrors(parser);

    if (program == nullptr) {
        FAIL() << "parseProgram() returned nullptr";
    }

    if (program->statements.size() != 1) {
        FAIL() << "program.statements does not contain 1 statement. got="
               << program->statements.size();
    }

    const auto &stmt = program->statements[0];
    const auto *exprStmt = std::get_if<ExpressionStatement>(&stmt);

    if (exprStmt == nullptr) {
        FAIL() << "stmt not ExpressionStatement. got=" << typeid(stmt).name();
    }

    testBooleanLiteral(exprStmt->expression, true);
}

TEST(ParserTest, PrefixExpressions) {
    std::vector<std::tuple<std::string, std::string, Literal>> prefixTests = {
        {"!5;", "!", 5},
        {"-15;", "-", 15},
        {"!true;", "!", true},
        {"!false;", "!", false},
    };

    for (const auto &[input, op, value] : prefixTests) {
        auto parser = Parser(std::make_unique<Lexer>(input));
        auto program = parser.parseProgram();

        checkParserErrors(parser);

        if (program == nullptr) {
            FAIL() << "parseProgram() returned nullptr";
        }

        if (program->statements.size() != 1) {
            FAIL() << "program.statements does not contain 1 statement. got="
                   << program->statements.size();
        }

        const auto &stmt = program->statements[0];
        const auto *exprStmt = std::get_if<ExpressionStatement>(&stmt);

        if (exprStmt == nullptr) {
            FAIL() << "stmt not ExpressionStatement. got=" << typeid(stmt).name();
        }

        const auto *prefixExpr =
            std::get_if<Box<PrefixExpression>>(&exprStmt->expression);

        if (prefixExpr == nullptr) {
            FAIL() << "expression not PrefixExpression. got="
                   << typeid(exprStmt->expression).name();
        }

        EXPECT_EQ((*prefixExpr)->op, op)
            << "operator is not '" << op << "'. got=" << (*prefixExpr)->op;

        testLiteralExpression((*prefixExpr)->right, value);
    }
}

TEST(ParserTest, InfixExpressions) {
    std::vector<std::tuple<std::string, Literal, std::string, Literal>> infixTests = {
        {"5 + 5;", 5, "+", 5},
        {"5 - 5;", 5, "-", 5},
        {"5 * 5;", 5, "*", 5},
        {"5 / 5;", 5, "/", 5},
        {"5 > 5;", 5, ">", 5},
        {"5 < 5;", 5, "<", 5},
        {"5 == 5;", 5, "==", 5},
        {"5 != 5;", 5, "!=", 5},
        {"foobar + barfoo;", "foobar", "+", "barfoo"},
        {"true == true", true, "==", true},
        {"true != false", true, "!=", false},
        {"false == false", false, "==", false},
    };

    for (const auto &[input, leftValue, op, rightValue] : infixTests) {
        auto parser = Parser(std::make_unique<Lexer>(input));
        auto program = parser.parseProgram();

        checkParserErrors(parser);

        if (program == nullptr) {
            FAIL() << "parseProgram() returned nullptr";
        }

        if (program->statements.size() != 1) {
            FAIL() << "program.statements does not contain 1 statement. got="
                   << program->statements.size();
        }

        const auto &stmt = program->statements[0];
        const auto *exprStmt = std::get_if<ExpressionStatement>(&stmt);

        if (exprStmt == nullptr) {
            FAIL() << "stmt not ExpressionStatement. got=" << typeid(stmt).name();
        }

        testInfixExpression(exprStmt->expression, leftValue, op, rightValue);
    }
}

TEST(ParserTest, OperatorPrecedenceParsing) {
    std::vector<std::tuple<std::string, std::string>> tests = {
        {"-a * b", "((-a) * b)"},
        {"!-a", "(!(-a))"},
        {"a + b + c", "((a + b) + c)"},
        {"a + b - c", "((a + b) - c)"},
        {"a * b * c", "((a * b) * c)"},
        {"a * b / c", "((a * b) / c)"},
        {"a + b / c", "(a + (b / c))"},
        {"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f)"},
        {"3 + 4; -5 * 5", "(3 + 4)((-5) * 5)"},
        {"5 > 4 == 3 < 4", "((5 > 4) == (3 < 4))"},
        {"5 < 4 != 3 > 4", "((5 < 4) != (3 > 4))"},
        {"true", "true"},
        {"false", "false"},
        {"3 > 5 == false", "((3 > 5) == false)"},
        {"3 < 5 == true", "((3 < 5) == true)"},
        {"1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)"},
        {"(5 + 5) * 2", "((5 + 5) * 2)"},
        {"2 / (5 + 5)", "(2 / (5 + 5))"},
        {"-(5 + 5)", "(-(5 + 5))"},
        {"!(true == true)", "(!(true == true))"},
        {"if (3 > 5) { 10 } else { 20 }", "if (3 > 5) { 10 } else { 20 }"},
        {"if (3 > 5) { 10 }", "if (3 > 5) { 10 }"},
        {"fn(x, y) { x + y }", "fn(x, y) { (x + y) }"},
    };

    for (const auto &[input, expected] : tests) {
        auto parser = Parser(std::make_unique<Lexer>(input));
        auto program = parser.parseProgram();

        checkParserErrors(parser);

        if (program == nullptr) {
            FAIL() << "parseProgram() returned nullptr";
        }

        EXPECT_EQ(toString(*program), expected)
            << "expected=" << expected << ", got=" << toString(*program);
    }
}

TEST(ParserTest, IfExpression) {
    std::string input = "if (x < y) { x }";

    auto parser = Parser(std::make_unique<Lexer>(input));
    auto program = parser.parseProgram();

    checkParserErrors(parser);

    if (program == nullptr) {
        FAIL() << "parseProgram() returned nullptr";
    }

    if (program->statements.size() != 1) {
        FAIL() << "program.statements does not contain 1 statement. got="
               << program->statements.size();
    }

    const auto &stmt = program->statements[0];
    const auto *exprStmt = std::get_if<ExpressionStatement>(&stmt);

    if (exprStmt == nullptr) {
        FAIL() << "stmt not ExpressionStatement. got=" << typeid(stmt).name();
    }

    const auto *boxIfExpr = std::get_if<Box<IfExpression>>(&exprStmt->expression);
    if (boxIfExpr == nullptr) {
        FAIL() << "expression not IfExpression. got="
               << typeid(exprStmt->expression).name();
    }

    const auto &ifExpr = *boxIfExpr;
    testInfixExpression(ifExpr->condition, "x", "<", "y");

    if (ifExpr->consequence.statements.size() != 1) {
        FAIL() << "consequence is not 1 statement. got="
               << ifExpr->consequence.statements.size();
    }

    const auto &consequenceStmt = ifExpr->consequence.statements[0];
    const auto *consequenceExprStmt = std::get_if<ExpressionStatement>(&consequenceStmt);

    if (consequenceExprStmt == nullptr) {
        FAIL() << "consequence stmt not ExpressionStatement. got="
               << typeid(consequenceStmt).name();
    }

    testIdentifier(consequenceExprStmt->expression, "x");

    EXPECT_FALSE(ifExpr->alternative.has_value()) << "alternative was not null";
}

TEST(ParserTest, IfElseExpression) {
    std::string input = "if (x < y) { x } else { y }";

    auto parser = Parser(std::make_unique<Lexer>(input));
    auto program = parser.parseProgram();

    checkParserErrors(parser);

    if (program == nullptr) {
        FAIL() << "parseProgram() returned nullptr";
    }

    if (program->statements.size() != 1) {
        FAIL() << "program.statements does not contain 1 statement. got="
               << program->statements.size();
    }

    const auto &stmt = program->statements[0];
    const auto *exprStmt = std::get_if<ExpressionStatement>(&stmt);

    if (exprStmt == nullptr) {
        FAIL() << "stmt not ExpressionStatement. got=" << typeid(stmt).name();
    }

    const auto *boxIfExpr = std::get_if<Box<IfExpression>>(&exprStmt->expression);
    if (boxIfExpr == nullptr) {
        FAIL() << "expression not IfExpression. got="
               << typeid(exprStmt->expression).name();
    }

    const auto &ifExpr = *boxIfExpr;
    testInfixExpression(ifExpr->condition, "x", "<", "y");

    if (ifExpr->consequence.statements.size() != 1) {
        FAIL() << "consequence is not 1 statement. got="
               << ifExpr->consequence.statements.size();
    }

    const auto &consequenceStmt = ifExpr->consequence.statements[0];
    const auto *consequenceExprStmt = std::get_if<ExpressionStatement>(&consequenceStmt);

    if (consequenceExprStmt == nullptr) {
        FAIL() << "consequence stmt not ExpressionStatement. got="
               << typeid(consequenceStmt).name();
    }

    testIdentifier(consequenceExprStmt->expression, "x");

    if (!ifExpr->alternative) {
        FAIL() << "alternative was null";
    }

    const auto &alternativeStmt = ifExpr->alternative->statements[0];
    const auto *alternativeExprStmt = std::get_if<ExpressionStatement>(&alternativeStmt);

    if (alternativeExprStmt == nullptr) {
        FAIL() << "alternative stmt not ExpressionStatement. got="
               << typeid(alternativeStmt).name();
    }

    testIdentifier(alternativeExprStmt->expression, "y");
}

TEST(ParserTest, FunctionLiteralParsing) {
    std::string input = "fn(x, y) { x + y; }";

    auto parser = Parser(std::make_unique<Lexer>(input));
    auto program = parser.parseProgram();

    checkParserErrors(parser);

    if (program == nullptr) {
        FAIL() << "parseProgram() returned nullptr";
    }

    if (program->statements.size() != 1) {
        FAIL() << "program.statements does not contain 1 statement. got="
               << program->statements.size();
    }

    const auto &stmt = program->statements[0];
    const auto *exprStmt = std::get_if<ExpressionStatement>(&stmt);

    if (exprStmt == nullptr) {
        FAIL() << "stmt not ExpressionStatement. got=" << typeid(stmt).name();
    }

    const auto *boxFuncLit = std::get_if<Box<FunctionLiteral>>(&exprStmt->expression);
    if (boxFuncLit == nullptr) {
        FAIL() << "expression not FunctionLiteral. got="
               << typeid(exprStmt->expression).name();
    }

    const auto &funcLit = *boxFuncLit;

    EXPECT_EQ(funcLit->parameters.size(), 2);
    testIdentifier(funcLit->parameters[0], "x");
    testIdentifier(funcLit->parameters[1], "y");

    EXPECT_EQ(funcLit->body.statements.size(), 1);

    const auto &bodyStmt = funcLit->body.statements[0];
    const auto *bodyExprStmt = std::get_if<ExpressionStatement>(&bodyStmt);

    if (bodyExprStmt == nullptr) {
        FAIL() << "body stmt not ExpressionStatement. got=" << typeid(bodyStmt).name();
    }

    testInfixExpression(bodyExprStmt->expression, "x", "+", "y");
}

TEST(ParserTest, FunctionLiteralParameterParsing) {
    std::vector<std::tuple<std::string, std::vector<std::string>>> tests = {
        {"fn() {};", {}},
        {"fn(x) {};", {"x"}},
        {"fn(x, y, z) {};", {"x", "y", "z"}},
    };

    for (const auto &[input, expectedParams] : tests) {
        auto parser = Parser(std::make_unique<Lexer>(input));
        auto program = parser.parseProgram();

        checkParserErrors(parser);

        if (program == nullptr) {
            FAIL() << "parseProgram() returned nullptr";
        }

        if (program->statements.size() != 1) {
            FAIL() << "program.statements does not contain 1 statement. got="
                   << program->statements.size();
        }

        const auto &stmt = program->statements[0];
        const auto *exprStmt = std::get_if<ExpressionStatement>(&stmt);

        if (exprStmt == nullptr) {
            FAIL() << "stmt not ExpressionStatement. got=" << typeid(stmt).name();
        }

        const auto *boxFuncLit = std::get_if<Box<FunctionLiteral>>(&exprStmt->expression);
        if (boxFuncLit == nullptr) {
            FAIL() << "expression not FunctionLiteral. got="
                   << typeid(exprStmt->expression).name();
        }

        const auto &funcLit = *boxFuncLit;
        EXPECT_EQ(funcLit->parameters.size(), expectedParams.size());

        for (size_t i = 0; i < expectedParams.size(); ++i) {
            testIdentifier(funcLit->parameters[i], expectedParams[i]);
        }
    }
}
