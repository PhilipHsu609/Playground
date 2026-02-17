#include "monkey/ast.h"
#include "monkey/lexer.h"
#include "monkey/parser.h"

#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

using namespace monkey; // NOLINT(google-build-using-namespace) - for cleaner test code

void checkParserErrors(const Parser &parser) {
    const auto &errors = parser.errors();
    if (errors.empty()) {
        return;
    }
    FAIL() << "parser has " << errors.size() << " errors:\n"
           << fmt::format("{}", fmt::join(errors, "\n"));
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
        EXPECT_EQ(tokenLiteral(letStmt->name), expectedIdentifiers[idx])
            << "let_stmt.name.tokenLiteral() not '" << expectedIdentifiers[idx]
            << "'. got=" << tokenLiteral(letStmt->name);
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

    const auto *ident = std::get_if<Identifier>(&exprStmt->expression);

    if (ident == nullptr) {
        FAIL() << "expression not Identifier. got="
               << typeid(exprStmt->expression).name();
    }

    EXPECT_EQ(tokenLiteral(*ident), "foobar")
        << "ident.tokenLiteral() not 'foobar'. got=" << tokenLiteral(*ident);
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

    const auto *intLiteral = std::get_if<IntegerLiteral>(&exprStmt->expression);

    if (intLiteral == nullptr) {
        FAIL() << "expression not IntegerLiteral. got="
               << typeid(exprStmt->expression).name();
    }

    EXPECT_EQ(tokenLiteral(*intLiteral), "5")
        << "intLiteral.tokenLiteral() not '5'. got=" << tokenLiteral(*intLiteral);
    EXPECT_EQ(intLiteral->value, 5)
        << "intLiteral.value not 5. got=" << intLiteral->value;
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

        const auto *intLiteral = std::get_if<IntegerLiteral>(&(*prefixExpr)->right);

        if (intLiteral == nullptr) {
            FAIL() << "right expression not IntegerLiteral. got="
                   << typeid((*prefixExpr)->right).name();
        }

        EXPECT_EQ(tokenLiteral(*intLiteral), std::to_string(value))
            << "intLiteral.tokenLiteral() not '" << value
            << "'. got=" << tokenLiteral(*intLiteral);
        EXPECT_EQ(intLiteral->value, value)
            << "intLiteral.value not " << value << ". got=" << intLiteral->value;
    }
}
