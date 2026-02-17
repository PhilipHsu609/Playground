#include "monkey/parser.h"

#include <fmt/format.h>
#include <magic_enum/magic_enum_format.hpp>

#include <memory>
#include <optional>
#include <utility>

namespace monkey {

Parser::Parser(std::unique_ptr<Lexer> lexer) : lexer_(std::move(lexer)) {
    // Initialize currentToken and peekToken
    nextToken();
    nextToken();
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();

    while (currentToken_.type != TokenType::EOF_) {
        if (auto stmt = parseStatement()) {
            program->statements.emplace_back(std::move(*stmt));
        }
        nextToken();
    }

    return program;
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

std::optional<Statement> Parser::parseStatement() {
    switch (currentToken_.type) {
    case TokenType::LET:
        return parseLetStatement();
    default:
        break;
    }
    return std::nullopt;
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

} // namespace monkey