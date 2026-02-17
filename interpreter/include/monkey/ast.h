#pragma once

#include "monkey/token.h"

#include <fmt/format.h>

#include <concepts>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace monkey {

template <typename T>
class Box {
  public:
    Box(T &&obj) // NOLINT(google-explicit-constructor)
                 // for easier construction of Box<T>
        : ptr_(std::make_unique<T>(std::move(obj))) {}
    Box(const T &obj) // NOLINT(google-explicit-constructor)
                      // for easier construction of Box<T>
        : ptr_(std::make_unique<T>(obj)) {}

    Box(const Box &other) : Box(*other.ptr_) {}
    Box &operator=(const Box &other) {
        if (this == &other) {
            return *this;
        }
        *ptr_ = *other.ptr_;
        return *this;
    }

    Box(Box &&) noexcept = default;
    Box &operator=(Box &&) noexcept = default;

    ~Box() = default;

    T &operator*() { return *ptr_; }
    const T &operator*() const { return *ptr_; }
    T *operator->() { return ptr_.get(); }
    const T *operator->() const { return ptr_.get(); }

  private:
    std::unique_ptr<T> ptr_;
};

// Recursive variant types forward declarations
struct PrefixExpression;
struct InfixExpression;

// Leaf expression types definitions
struct Identifier {
    Token token;
};

struct IntegerLiteral {
    Token token;
    int64_t value;
};

using Expression =
    std::variant<Identifier, IntegerLiteral, Box<PrefixExpression>, Box<InfixExpression>>;

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

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

std::string toString(const Program &program);
std::string toString(const Expression &expr);
std::string toString(const Statement &stmt);

} // namespace monkey
