#include "monkey/parser.h"
#include "monkey/ast.h"
#include "monkey/token.h"

#include <fmt/format.h>
#include <magic_enum/magic_enum_format.hpp>

#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace monkey {

const static std::unordered_map<TokenType, Precedence> PRECEDENCES = {
    {TokenType::EQ, Precedence::EQUALS},      {TokenType::NOT_EQ, Precedence::EQUALS},
    {TokenType::LT, Precedence::LESSGREATER}, {TokenType::GT, Precedence::LESSGREATER},
    {TokenType::PLUS, Precedence::SUM},       {TokenType::MINUS, Precedence::SUM},
    {TokenType::SLASH, Precedence::PRODUCT},  {TokenType::ASTERISK, Precedence::PRODUCT},
    {TokenType::LPAREN, Precedence::CALL},
};

Parser::Parser(std::unique_ptr<Lexer> lexer) : lexer_(std::move(lexer)) {
    // Initialize currentToken and peekToken
    nextToken();
    nextToken();

    // Register prefix parse functions
    registerPrefix(TokenType::IDENT, [this]() { return this->parseIdentifier(); });
    registerPrefix(TokenType::INT, [this]() { return this->parseIntegerLiteral(); });
    registerPrefix(TokenType::TRUE, [this]() { return this->parseBoolean(); });
    registerPrefix(TokenType::FALSE, [this]() { return this->parseBoolean(); });
    registerPrefix(TokenType::BANG, [this]() { return this->parsePrefixExpression(); });
    registerPrefix(TokenType::MINUS, [this]() { return this->parsePrefixExpression(); });
    registerPrefix(TokenType::LPAREN,
                   [this]() { return this->parseGroupedExpression(); });
    registerPrefix(TokenType::IF, [this]() { return this->parseIfExpression(); });
    registerPrefix(TokenType::FUNCTION,
                   [this]() { return this->parseFunctionLiteral(); });

    // Register infix parse functions
    auto infixParseFn = [this](Expression left) {
        return this->parseInfixExpression(std::move(left));
    };
    registerInfix(TokenType::PLUS, infixParseFn);
    registerInfix(TokenType::MINUS, infixParseFn);
    registerInfix(TokenType::SLASH, infixParseFn);
    registerInfix(TokenType::ASTERISK, infixParseFn);
    registerInfix(TokenType::EQ, infixParseFn);
    registerInfix(TokenType::NOT_EQ, infixParseFn);
    registerInfix(TokenType::LT, infixParseFn);
    registerInfix(TokenType::GT, infixParseFn);
    registerInfix(TokenType::LPAREN, [this](Expression left) {
        return this->parseCallExpression(std::move(left));
    });
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();

    while (currentToken_.type != TokenType::EOF_TOKEN) {
        if (auto stmt = parseStatement()) {
            program->statements.emplace_back(std::move(*stmt));
        }
        nextToken();
    }

    return program;
}

void Parser::registerPrefix(TokenType tokenType, PrefixParseFn fn) {
    prefixParseFns_[tokenType] = std::move(fn);
}

void Parser::registerInfix(TokenType tokenType, InfixParseFn fn) {
    infixParseFns_[tokenType] = std::move(fn);
}

void Parser::nextToken() {
    currentToken_ = peekToken_;
    peekToken_ = lexer_->nextToken();
}

bool Parser::expectPeek(TokenType type) {
    if (peekToken_.type == type) {
        nextToken();
        return true;
    }
    peekError(type);
    return false;
}

void Parser::peekError(TokenType type) {
    auto error = fmt::format("expected next token to be {}, got {} instead", type,
                             peekToken_.type);
    errors_.push_back(error);
}

Precedence Parser::peekPrecedence() const {
    if (auto it = PRECEDENCES.find(peekToken_.type); it != PRECEDENCES.end()) {
        return it->second;
    }
    return Precedence::LOWEST;
}

Precedence Parser::currentPrecedence() const {
    if (auto it = PRECEDENCES.find(currentToken_.type); it != PRECEDENCES.end()) {
        return it->second;
    }
    return Precedence::LOWEST;
}

std::optional<Statement> Parser::parseStatement() {
    switch (currentToken_.type) {
    case TokenType::LET:
        return parseLetStatement();
    case TokenType::RETURN:
        return parseReturnStatement();
    default:
        return parseExpressionStatement();
    }
}

std::optional<Statement> Parser::parseLetStatement() {
    auto stmt = LetStatement{.token = currentToken_, .name = {}, .value = {}};

    if (!expectPeek(TokenType::IDENT)) {
        return std::nullopt;
    }

    stmt.name = Identifier{.token = currentToken_};

    if (!expectPeek(TokenType::ASSIGN)) {
        return std::nullopt;
    }

    // TODO: Skip expressions until we encounter a semicolon
    while (currentToken_.type != TokenType::SEMICOLON) {
        nextToken();
    }

    return stmt;
}

std::optional<Statement> Parser::parseReturnStatement() {
    auto stmt = ReturnStatement{.token = currentToken_, .value = {}};

    // TODO: Skip expressions until we encounter a semicolon
    nextToken();
    while (currentToken_.type != TokenType::SEMICOLON) {
        nextToken();
    }

    return stmt;
}

std::optional<Statement> Parser::parseExpressionStatement() {
    auto stmt = ExpressionStatement{.token = currentToken_, .expression = {}};
    auto expr = parseExpression(Precedence::LOWEST);

    if (!expr) {
        return std::nullopt;
    }

    stmt.expression = std::move(*expr);

    if (peekToken_.type == TokenType::SEMICOLON) {
        // Optional semicolon, e.g., 5 + 5 in REPL
        nextToken();
    }

    return stmt;
}

std::optional<BlockStatement> Parser::parseBlockStatement() {
    auto block = BlockStatement{.token = currentToken_, .statements = {}};

    nextToken();
    while (currentToken_.type != TokenType::RBRACE &&
           currentToken_.type != TokenType::EOF_TOKEN) {
        if (auto stmt = parseStatement()) {
            block.statements.emplace_back(std::move(*stmt));
        }
        nextToken();
    }
    return block;
}

std::optional<Expression> Parser::parseExpression(Precedence precedence) {
    // Use the expression -5 + 5 * 10 as an example to understand how this works

    // 1. We start with the first token, which is '-'.
    auto prefixParseFnIt = prefixParseFns_.find(currentToken_.type);
    if (prefixParseFnIt == prefixParseFns_.end()) {
        auto error =
            fmt::format("no prefix parse function for {} found", currentToken_.literal);
        errors_.push_back(error);
        return std::nullopt;
    }

    // We look up the prefix parse function for '-' and call it,
    // which returns a PrefixExpression with the right side being an IntegerLiteral and
    // consumes both the token '-' and the token '5'.
    auto leftExpr = prefixParseFnIt->second();
    if (!leftExpr) {
        return std::nullopt;
    }

    // 2. Now we look at the next token, which is '+'.
    // We check if its precedence is higher than the current precedence (LOWEST).
    while (peekToken_.type != TokenType::SEMICOLON && precedence < peekPrecedence()) {
        // 3. Since '+' has higher precedence than LOWEST, we look up its infix parse
        // function.
        auto infixParseFnIt = infixParseFns_.find(peekToken_.type);
        if (infixParseFnIt == infixParseFns_.end()) {
            return leftExpr;
        }
        // 4. We consume the '+' token (make it current) and parse the right side of the
        // expression.
        nextToken();
        auto newLeftExpr = infixParseFnIt->second(std::move(*leftExpr));
        if (!newLeftExpr) {
            return std::nullopt;
        }
        // 5. The infix parse function returns a new expression that combines the left and
        // right sides with the operator, e.g., left(-5) + right(5 * 10). We then repeat
        // the process, now using this new expression as the left side.
        leftExpr = std::move(newLeftExpr);
    }

    return leftExpr;
}

std::optional<Expression> Parser::parseIdentifier() {
    return Identifier{.token = currentToken_};
}

std::optional<Expression> Parser::parseIntegerLiteral() {
    try {
        auto value = std::stoll(currentToken_.literal);
        return IntegerLiteral{.token = currentToken_, .value = value};
    } catch (const std::invalid_argument &e) {
        auto error = fmt::format("could not parse {} as integer", currentToken_.literal);
        errors_.push_back(error);
    } catch (const std::out_of_range &e) {
        auto error = fmt::format("{} is out of range for int64_t", currentToken_.literal);
        errors_.push_back(error);
    }
    return std::nullopt;
}

std::optional<Expression> Parser::parseFunctionLiteral() {
    auto func = FunctionLiteral{.token = currentToken_, .parameters = {}, .body = {}};

    if (!expectPeek(TokenType::LPAREN)) {
        return std::nullopt;
    }

    if (peekToken_.type != TokenType::RPAREN) {
        // Parse the first parameter.
        nextToken();
        func.parameters.emplace_back(Identifier{.token = currentToken_});

        // Parse additional parameters, if any.
        while (peekToken_.type == TokenType::COMMA) {
            nextToken();
            nextToken();
            func.parameters.emplace_back(Identifier{.token = currentToken_});
        }
    }

    if (!expectPeek(TokenType::RPAREN)) {
        return std::nullopt;
    }

    if (!expectPeek(TokenType::LBRACE)) {
        return std::nullopt;
    }

    auto body = parseBlockStatement();
    if (!body) {
        return std::nullopt;
    }
    func.body = std::move(*body);

    return func;
}

std::optional<Expression> Parser::parseBoolean() {
    return BooleanLiteral{.token = currentToken_,
                          .value = currentToken_.type == TokenType::TRUE};
}

std::optional<Expression> Parser::parsePrefixExpression() {
    auto expr = PrefixExpression{
        .token = currentToken_, .op = currentToken_.literal, .right = {}};
    nextToken();
    auto right = parseExpression(Precedence::PREFIX);
    if (!right) {
        return std::nullopt;
    }
    expr.right = std::move(*right);
    return expr;
}

std::optional<Expression> Parser::parseInfixExpression(Expression left) {
    auto expr = InfixExpression{.token = currentToken_,
                                .left = std::move(left),
                                .op = currentToken_.literal,
                                .right = {}};
    auto precedence = currentPrecedence();
    nextToken();
    auto right = parseExpression(precedence);
    if (!right) {
        return std::nullopt;
    }
    expr.right = std::move(*right);
    return expr;
}

std::optional<Expression> Parser::parseGroupedExpression() {
    nextToken();
    auto expr = parseExpression(Precedence::LOWEST);
    if (!expr) {
        return std::nullopt;
    }
    if (!expectPeek(TokenType::RPAREN)) {
        return std::nullopt;
    }
    return expr;
}

std::optional<Expression> Parser::parseIfExpression() {
    auto expr = IfExpression{.token = currentToken_,
                             .condition = {},
                             .consequence = {},
                             .alternative = std::nullopt};

    if (!expectPeek(TokenType::LPAREN)) {
        return std::nullopt;
    }

    nextToken();
    auto condition = parseExpression(Precedence::LOWEST);
    if (!condition) {
        return std::nullopt;
    }
    expr.condition = std::move(*condition);

    if (!expectPeek(TokenType::RPAREN)) {
        return std::nullopt;
    }

    if (!expectPeek(TokenType::LBRACE)) {
        return std::nullopt;
    }

    auto consequence = parseBlockStatement();
    if (!consequence) {
        return std::nullopt;
    }
    expr.consequence = std::move(*consequence);

    if (peekToken_.type == TokenType::ELSE) {
        nextToken();

        if (!expectPeek(TokenType::LBRACE)) {
            return std::nullopt;
        }

        auto alternative = parseBlockStatement();
        if (!alternative) {
            return std::nullopt;
        }
        expr.alternative = std::move(*alternative);
    }

    return expr;
}

std::optional<Expression> Parser::parseCallExpression(Expression function) {
    auto expr = CallExpression{
        .token = currentToken_, .function = std::move(function), .arguments = {}};

    if (peekToken_.type == TokenType::RPAREN) {
        nextToken();
        return expr;
    }

    // Parse the first argument.
    nextToken();
    auto arg = parseExpression(Precedence::LOWEST);
    if (!arg) {
        return std::nullopt;
    }
    expr.arguments.emplace_back(std::move(*arg));

    // Parse additional arguments, if any.
    while (peekToken_.type == TokenType::COMMA) {
        nextToken();
        nextToken();
        arg = parseExpression(Precedence::LOWEST);
        if (!arg) {
            return std::nullopt;
        }
        expr.arguments.emplace_back(std::move(*arg));
    }

    if (!expectPeek(TokenType::RPAREN)) {
        return std::nullopt;
    }

    return expr;
}

} // namespace monkey
