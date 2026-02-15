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
    MINUS,
    BANG,
    ASTERISK,
    SLASH,
    LT,
    GT,
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
    TRUE,
    FALSE,
    IF,
    ELSE,
    RETURN,
};

struct Token {
    TokenType type;
    std::string literal;
};

inline TokenType lookupIdent(std::string_view ident) {
    using namespace std::literals;

    constexpr auto keywords = []() {
        auto arr = std::to_array<std::pair<std::string_view, TokenType>>({
            {"fn"sv, TokenType::FUNCTION},
            {"let"sv, TokenType::LET},
            {"true"sv, TokenType::TRUE},
            {"false"sv, TokenType::FALSE},
            {"if"sv, TokenType::IF},
            {"else"sv, TokenType::ELSE},
            {"return"sv, TokenType::RETURN},
        });
        std::ranges::sort(arr, std::ranges::less{},
                          &std::pair<std::string_view, TokenType>::first);
        return arr;
    }();

    static_assert(std::ranges::is_sorted(keywords, std::ranges::less{},
                                         &std::pair<std::string_view, TokenType>::first),
                  "keywords array must be sorted by the first element of the pair");

    const auto *it =
        std::ranges::lower_bound(keywords, ident, std::ranges::less{},
                                 &std::pair<std::string_view, TokenType>::first);

    if (it != keywords.end() && it->first == ident) {
        return it->second;
    }
    return TokenType::IDENT;
}

} // namespace monkey
