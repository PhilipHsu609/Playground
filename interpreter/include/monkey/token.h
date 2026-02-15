#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace monkey {

enum class TokenType {
    // Special
    ILLEGAL,
    EOF_,
    // Identifiers + literals
    IDENT,
    INT,
    // Operators
    ASSIGN,
    PLUS,
    // Delimiters
    COMMA,
    SEMICOLON,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    // Keywords
    FUNCTION,
    LET,
};

struct Token {
    TokenType type;
    std::string literal;
};

inline TokenType lookupIdent(std::string_view ident) {
    using namespace std::literals;

    static const std::unordered_map<std::string_view, TokenType> keywords{
        {"fn"sv, TokenType::FUNCTION},
        {"let"sv, TokenType::LET},
    };

    auto it = keywords.find(ident);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::IDENT;
}

} // namespace monkey
