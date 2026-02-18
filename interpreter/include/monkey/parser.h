#pragma once

#include "monkey/ast.h"
#include "monkey/lexer.h"
#include "monkey/token.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace monkey {

enum class Precedence {
    LOWEST = 1,
    EQUALS,      // ==
    LESSGREATER, // > or <
    SUM,         // +
    PRODUCT,     // *
    PREFIX,      // -X or !X
    CALL,        // myFunction(X)
};

class Parser {
  public:
    explicit Parser(std::unique_ptr<Lexer> lexer);
    std::unique_ptr<Program> parseProgram();
    const std::vector<std::string> &errors() const { return errors_; }

  private:
    using PrefixParseFn = std::function<std::optional<Expression>()>;
    using InfixParseFn = std::function<std::optional<Expression>(Expression lhs)>;

    void nextToken();
    bool expectPeek(TokenType type);
    void peekError(TokenType type);

    Precedence peekPrecedence() const;
    Precedence currentPrecedence() const;

    std::optional<Statement> parseStatement();
    std::optional<Statement> parseLetStatement();
    std::optional<Statement> parseReturnStatement();
    std::optional<Statement> parseExpressionStatement();
    std::optional<BlockStatement> parseBlockStatement();

    std::optional<Expression> parseExpression(Precedence precedence);
    std::optional<Expression> parseIdentifier();
    std::optional<Expression> parseBoolean();
    std::optional<Expression> parseIntegerLiteral();
    std::optional<Expression> parseFunctionLiteral();
    std::optional<Expression> parsePrefixExpression();
    std::optional<Expression> parseInfixExpression(Expression left);
    std::optional<Expression> parseGroupedExpression();
    std::optional<Expression> parseIfExpression();
    std::optional<Expression> parseCallExpression(Expression function);

    void registerPrefix(TokenType tokenType, PrefixParseFn fn);
    void registerInfix(TokenType tokenType, InfixParseFn fn);

    std::unique_ptr<Lexer> lexer_;
    Token currentToken_;
    Token peekToken_;

    std::unordered_map<TokenType, PrefixParseFn> prefixParseFns_;
    std::unordered_map<TokenType, InfixParseFn> infixParseFns_;

    std::vector<std::string> errors_;
};

} // namespace monkey