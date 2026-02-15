#include "monkey/lexer.h"
#include "monkey/token.h"

#include <cctype>
#include <fmt/format.h>

namespace {
bool isLetter(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}
bool isDigit(char ch) { return std::isdigit(static_cast<unsigned char>(ch)) != 0; }
bool isWhitespace(char ch) { return std::isspace(static_cast<unsigned char>(ch)) != 0; }
} // namespace

namespace monkey {

void Lexer::readChar() {
    if (read_position_ >= input_.size()) {
        ch_ = 0; // ASCII code for NUL, signifies end of input
    } else {
        ch_ = input_[read_position_];
    }
    position_ = read_position_++;
}

Token Lexer::nextToken() {
    Token token{};

    skipWhitespace();

    switch (ch_) {
    case '=':
        token = {TokenType::ASSIGN, "="};
        break;
    case ';':
        token = {TokenType::SEMICOLON, ";"};
        break;
    case '(':
        token = {TokenType::LPAREN, "("};
        break;
    case ')':
        token = {TokenType::RPAREN, ")"};
        break;
    case '{':
        token = {TokenType::LBRACE, "{"};
        break;
    case '}':
        token = {TokenType::RBRACE, "}"};
        break;
    case ',':
        token = {TokenType::COMMA, ","};
        break;
    case '+':
        token = {TokenType::PLUS, "+"};
        break;
    case 0:
        token = {TokenType::EOF_, ""};
        break;
    default:
        if (isLetter(ch_)) {
            std::string ident = readIdentifier();
            return {lookupIdent(ident), ident};
        } else if (isDigit(ch_)) {
            std::string number = readNumber();
            return {TokenType::INT, number};
        } else {
            token = {TokenType::ILLEGAL, {ch_}};
        }
    }

    readChar();
    return token;
}

std::string Lexer::readIdentifier() {
    auto start = position_;
    while (isLetter(ch_)) {
        readChar();
    }
    return input_.substr(start, position_ - start);
}

std::string Lexer::readNumber() {
    auto start = position_;
    while (isDigit(ch_)) {
        readChar();
    }
    return input_.substr(start, position_ - start);
}

void Lexer::skipWhitespace() {
    while (isWhitespace(ch_)) {
        readChar();
    }
}

} // namespace monkey