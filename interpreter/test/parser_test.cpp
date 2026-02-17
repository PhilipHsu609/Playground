#include "monkey/ast.h"
#include "monkey/lexer.h"
#include "monkey/parser.h"

#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include <ranges>
#include <string>
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

    std::vector<std::string> expected_identifiers{"x", "y", "foobar"};
    // std::vector<std::string> expected_values{"5", "10", "838383"};

    for (const auto &[i, stmt] : std::views::enumerate(program->statements)) {
        auto idx = static_cast<size_t>(i);
        const auto *let_stmt = std::get_if<LetStatement>(&stmt);

        if (let_stmt == nullptr) {
            FAIL() << "stmt not LetStatement. got=" << typeid(stmt).name();
        }

        EXPECT_EQ(tokenLiteral(*let_stmt), "let")
            << "let_stmt.tokenLiteral() not 'let'. got=" << tokenLiteral(*let_stmt);
        EXPECT_EQ(tokenLiteral(let_stmt->name), expected_identifiers[idx])
            << "let_stmt.name.tokenLiteral() not '" << expected_identifiers[idx]
            << "'. got=" << tokenLiteral(let_stmt->name);
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
        const auto *return_stmt = std::get_if<ReturnStatement>(&stmt);

        if (return_stmt == nullptr) {
            FAIL() << "stmt not ReturnStatement. got=" << typeid(stmt).name();
        }

        EXPECT_EQ(tokenLiteral(*return_stmt), "return")
            << "return_stmt.tokenLiteral() not 'return'. got="
            << tokenLiteral(*return_stmt);
        // EXPECT_EQ(tokenLiteral(return_stmt->value), expected_values[idx])
        //     << "return_stmt.value not '" << expected_values[idx]
        //     << "'. got=" << tokenLiteral(return_stmt->value);
    }
}