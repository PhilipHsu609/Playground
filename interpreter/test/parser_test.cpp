#include "monkey/ast.h"
#include "monkey/lexer.h"
#include "monkey/parser.h"

#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <ranges>
#include <string>
#include <tuple>
#include <type_traits>
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
    } else if constexpr (std::is_integral_v<T>) {
        testIntegerLiteral(expr, static_cast<int64_t>(expected));
    } else if constexpr (std::is_convertible_v<T, std::string>) {
        testIdentifier(expr, expected);
    } else {
        static_assert(always_false<T>, "unsupported literal type");
    }
}

void testInfixExpression(const Expression &expr, const auto &leftValue,
                         const std::string &op, const auto &rightValue) {
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

TEST(ParserTest, PrefixExpressions) {
    std::vector<std::tuple<std::string, std::string, int64_t>> prefixTests = {
        {"!5;", "!", 5},
        {"-15;", "-", 15},
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

        testIntegerLiteral((*prefixExpr)->right, value);
    }
}

TEST(ParserTest, InfixExpressions) {
    std::vector<std::tuple<std::string, int64_t, std::string, int64_t>> infixTests = {
        {"5 + 5;", 5, "+", 5},   {"5 - 5;", 5, "-", 5},   {"5 * 5;", 5, "*", 5},
        {"5 / 5;", 5, "/", 5},   {"5 > 5;", 5, ">", 5},   {"5 < 5;", 5, "<", 5},
        {"5 == 5;", 5, "==", 5}, {"5 != 5;", 5, "!=", 5},
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
