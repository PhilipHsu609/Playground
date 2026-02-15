#include "monkey/lexer.h"
#include "monkey/token.h"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <magic_enum/magic_enum_format.hpp>

#include <string>
#include <vector>

using namespace monkey; // NOLINT(google-build-using-namespace) - for cleaner test code

TEST(LexerTest, NextToken) {
    std::string input = R"(let five = 5;

let ten = 10;
let add = fn(x, y) {
    x + y;
};

let result = add(five, ten);
!-/*5;
5 < 10 > 5;

if (5 < 10) {
    return true;
} else {
    return false;
}

10 == 10;
10 != 9;
)";

    std::vector<Token> expected_tokens{
        {TokenType::LET, "let"},     {TokenType::IDENT, "five"},
        {TokenType::ASSIGN, "="},    {TokenType::INT, "5"},
        {TokenType::SEMICOLON, ";"},

        {TokenType::LET, "let"},     {TokenType::IDENT, "ten"},
        {TokenType::ASSIGN, "="},    {TokenType::INT, "10"},
        {TokenType::SEMICOLON, ";"},

        {TokenType::LET, "let"},     {TokenType::IDENT, "add"},
        {TokenType::ASSIGN, "="},    {TokenType::FUNCTION, "fn"},
        {TokenType::LPAREN, "("},    {TokenType::IDENT, "x"},
        {TokenType::COMMA, ","},     {TokenType::IDENT, "y"},
        {TokenType::RPAREN, ")"},    {TokenType::LBRACE, "{"},
        {TokenType::IDENT, "x"},     {TokenType::PLUS, "+"},
        {TokenType::IDENT, "y"},     {TokenType::SEMICOLON, ";"},
        {TokenType::RBRACE, "}"},    {TokenType::SEMICOLON, ";"},

        {TokenType::LET, "let"},     {TokenType::IDENT, "result"},
        {TokenType::ASSIGN, "="},    {TokenType::IDENT, "add"},
        {TokenType::LPAREN, "("},    {TokenType::IDENT, "five"},
        {TokenType::COMMA, ","},     {TokenType::IDENT, "ten"},
        {TokenType::RPAREN, ")"},    {TokenType::SEMICOLON, ";"},

        {TokenType::BANG, "!"},      {TokenType::MINUS, "-"},
        {TokenType::SLASH, "/"},     {TokenType::ASTERISK, "*"},
        {TokenType::INT, "5"},       {TokenType::SEMICOLON, ";"},

        {TokenType::INT, "5"},       {TokenType::LT, "<"},
        {TokenType::INT, "10"},      {TokenType::GT, ">"},
        {TokenType::INT, "5"},       {TokenType::SEMICOLON, ";"},

        {TokenType::IF, "if"},       {TokenType::LPAREN, "("},
        {TokenType::INT, "5"},       {TokenType::LT, "<"},
        {TokenType::INT, "10"},      {TokenType::RPAREN, ")"},
        {TokenType::LBRACE, "{"},    {TokenType::RETURN, "return"},
        {TokenType::TRUE, "true"},   {TokenType::SEMICOLON, ";"},
        {TokenType::RBRACE, "}"},    {TokenType::ELSE, "else"},
        {TokenType::LBRACE, "{"},    {TokenType::RETURN, "return"},
        {TokenType::FALSE, "false"}, {TokenType::SEMICOLON, ";"},
        {TokenType::RBRACE, "}"},

        {TokenType::INT, "10"},      {TokenType::EQ, "=="},
        {TokenType::INT, "10"},      {TokenType::SEMICOLON, ";"},
        {TokenType::INT, "10"},      {TokenType::NOT_EQ, "!="},
        {TokenType::INT, "9"},       {TokenType::SEMICOLON, ";"},

        {TokenType::EOF_, ""},
    };

    Lexer lexer(input);

    for (const auto &expected_token : expected_tokens) {
        Token token = lexer.nextToken();
        EXPECT_EQ(token.type, expected_token.type)
            << fmt::format("Token type mismatch: expected '{}', got '{}'",
                           expected_token.type, token.type);
        EXPECT_EQ(token.literal, expected_token.literal)
            << fmt::format("Token literal mismatch: expected '{}', got '{}'",
                           expected_token.literal, token.literal);
    }
}
