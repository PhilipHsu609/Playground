#pragma once

#include "monkey/box.h"
#include "monkey/token.h"

#include <fmt/format.h>

#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace monkey {

// Recursive variant types forward declarations
struct PrefixExpression;
struct InfixExpression;
struct IfExpression;
struct BlockStatement;
struct FunctionLiteral;
struct CallExpression;

// Leaf expression types definitions
struct Identifier {
    Token token;
};

struct IntegerLiteral {
    Token token;
    int64_t value;
};

struct BooleanLiteral {
    Token token;
    bool value = false;
};

struct StringLiteral {
    Token token;
    std::string value;
};

using Expression =
    std::variant<Identifier, IntegerLiteral, BooleanLiteral, StringLiteral,
                 Box<PrefixExpression>, Box<InfixExpression>, Box<IfExpression>,
                 Box<FunctionLiteral>, Box<CallExpression>>;

// Recursive expression types definitions
struct PrefixExpression {
    Token token;
    std::string op;
    Expression right;
};

struct InfixExpression {
    Token token;
    Expression left;
    std::string op;
    Expression right;
};

struct CallExpression {
    Token token; // The '(' token
    Expression function;
    std::vector<Expression> arguments;
};

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

using Statement =
    std::variant<LetStatement, ReturnStatement, ExpressionStatement, BlockStatement>;

struct BlockStatement {
    Token token; // The { token
    std::vector<Statement> statements;
};

struct IfExpression {
    Token token;
    Expression condition;
    BlockStatement consequence;
    std::optional<BlockStatement> alternative;
};

struct FunctionLiteral {
    Token token; // The 'fn' token
    std::vector<Identifier> parameters;
    BlockStatement body;
};

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

template <typename T>
struct is_box : std::false_type {};

template <typename T>
struct is_box<Box<T>> : std::true_type {};

template <typename T>
concept Boxed = is_box<std::remove_cvref_t<T>>::value;

std::string tokenLiteral(const Program &program);
std::string tokenLiteral(const HasToken auto &node) { return node.token.literal; }
std::string tokenLiteral(const Boxed auto &box) { return tokenLiteral(*box); }
std::string tokenLiteral(const Variant auto &var) {
    return std::visit([](const auto &s) { return tokenLiteral(s); }, var);
}

std::string toString(const Program &program);
std::string toString(const Expression &expr);
std::string toString(const Statement &stmt);

} // namespace monkey
