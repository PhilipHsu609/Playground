#pragma once

#include "monkey/token.h"

#include <string>
#include <variant>
#include <vector>

namespace monkey {

// TODO: Implement Box<T> for recursive data structures

// TODO: Forward declare Expression and Statement for recursive data structures

// Leaf expression types definitions
struct Identifier {
    std::string tokenLiteral() const { return token.literal; }
    Token token;
};

using Expression = std::variant<Identifier>;

// TODO: Recursive expression types definitions

// Statement types definitions
struct LetStatement {
    std::string tokenLiteral() const { return token.literal; }
    Token token;
    Identifier name;
    Expression value;
};

struct ReturnStatement {
    std::string tokenLiteral() const { return token.literal; }
    Token token;
    Expression value;
};

using Statement = std::variant<LetStatement, ReturnStatement>;

// Program is the root node of the AST
struct Program {
    std::string tokenLiteral() const;
    std::vector<Statement> statements;
};

// Helper function to get the token literal of an Expression or Statement
std::string tokenLiteral(const auto &node) {
    return std::visit([](const auto &s) { return s.tokenLiteral(); }, node);
}

} // namespace monkey
