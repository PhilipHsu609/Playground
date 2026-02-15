#include "monkey/lexer.h"
#include "monkey/token.h"

namespace {
constexpr bool isLetter(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}
constexpr bool isDigit(char ch) { return (ch >= '0' && ch <= '9'); }
constexpr bool isWhitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}
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
            std::string ident = readWhile(isLetter);
            return {lookupIdent(ident), ident};
        } else if (isDigit(ch_)) {
            std::string number = readWhile(isDigit);
            return {TokenType::INT, number};
        } else {
            token = {TokenType::ILLEGAL, {ch_}};
        }
    }

    readChar();
    return token;
}

std::string Lexer::readWhile(bool (*condition)(char)) {
    auto start = position_;
    while (condition(ch_)) {
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