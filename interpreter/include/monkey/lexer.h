#pragma once

#include "monkey/token.h"

#include <string>
#include <utility>

namespace monkey {

class Lexer {
  public:
    explicit Lexer(std::string input) : input_(std::move(input)) { readChar(); }
    Token nextToken();

  private:
    void readChar();
    char peekChar() const;
    void skipWhitespace();
    std::string readWhile(bool (*condition)(char));

    std::string input_;
    size_t position_{0};      // current position in input (points to current char)
    size_t read_position_{0}; // current reading position in input (after current char)
    char ch_{0};
};

} // namespace monkey