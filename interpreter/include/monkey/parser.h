#pragma once

#include "monkey/ast.h"
#include "monkey/lexer.h"
#include "monkey/token.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace monkey {

class Parser {
  public:
    Parser(std::unique_ptr<Lexer> lexer);
    std::unique_ptr<Program> parseProgram();
    const std::vector<std::string> &errors() const { return errors_; }

  private:
    void nextToken();
    bool expectPeek(TokenType type);
    void peekError(TokenType type);
    std::optional<Statement> parseStatement();
    std::optional<Statement> parseLetStatement();

    std::unique_ptr<Lexer> lexer_;
    Token currentToken_;
    Token peekToken_;

    std::vector<std::string> errors_;
};

} // namespace monkey