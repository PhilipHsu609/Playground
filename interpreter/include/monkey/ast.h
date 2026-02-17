#pragma once

#include "monkey/token.h"

#include <fmt/format.h>

#include <concepts>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace monkey {

// TODO: Implement Box<T> for recursive data structures

// TODO: Forward declare Expression and Statement for recursive data structures

// Leaf expression types definitions
struct Identifier {
    Token token;
};

using Expression = std::variant<Identifier>;

// TODO: Recursive expression types definitions

// Statement types definitions
struct LetStatement {
    Token token;
    Identifier name;
    Expression value;
};

struct ReturnStatement {
    Token token;
    Expression value;
};

// Think of ExpressionStatement as a wrapper for expressions that appear as statements.
// e.g., 5+5; or add(5, 10);
struct ExpressionStatement {
    Token token;
    Expression expression;
};

using Statement = std::variant<LetStatement, ReturnStatement, ExpressionStatement>;

// Program is the root node of the AST
struct Program {
    std::vector<Statement> statements;
};

// Helper functions
template <typename T>
struct is_variant : std::false_type {};

template <typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

template <typename T>
concept Variant = is_variant<std::remove_cvref_t<T>>::value;

template <typename T>
concept HasToken = requires(const T &t) {
    { t.token } -> std::convertible_to<Token>;
};

std::string tokenLiteral(const Program &program);
std::string tokenLiteral(const HasToken auto &node) { return node.token.literal; }
std::string tokenLiteral(const Variant auto &var) {
    return std::visit([](const auto &s) { return s.token.literal; }, var);
}

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

std::string toString(const Expression &expr);
std::string toString(const Statement &stmt);

} // namespace monkey
