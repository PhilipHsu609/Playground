#include "monkey/ast.h"

#include <gtest/gtest.h>

using namespace monkey; // NOLINT(google-build-using-namespace) - for cleaner test code

TEST(AstTest, ToString) {
    Statement stmt = LetStatement{
        .token = Token{.type = TokenType::LET, .literal = "let"},
        .name = Identifier{.token = Token{.type = TokenType::IDENT, .literal = "myVar"}},
        .value =
            Identifier{.token = Token{.type = TokenType::IDENT, .literal = "anotherVar"}},
    };

    EXPECT_EQ(toString(stmt), "let myVar = anotherVar;");

    Expression expr = Expression{PrefixExpression{
        .token = Token{.type = TokenType::MINUS, .literal = "-"},
        .operator_ = "-",
        .right = Identifier{.token = Token{.type = TokenType::IDENT, .literal = "5"}},
    }};

    EXPECT_EQ(toString(expr), "(-5)");
}