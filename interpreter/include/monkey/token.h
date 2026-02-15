#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

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

    constexpr auto keywords = std::to_array<std::pair<std::string_view, TokenType>>({
        {"fn"sv, TokenType::FUNCTION},
        {"let"sv, TokenType::LET},
    });

    const auto *it =
        std::ranges::lower_bound(keywords, ident, std::ranges::less{},
                                 &std::pair<std::string_view, TokenType>::first);

    if (it != keywords.end() && it->first == ident) {
        return it->second;
    }
    return TokenType::IDENT;
}

} // namespace monkey
