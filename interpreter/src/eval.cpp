#include "monkey/eval.h"
#include "monkey/ast.h"
#include "monkey/object.h"
#include "monkey/overload.h"

#include <variant>

namespace {

using namespace monkey;

Object evalPrefixExpression(const PrefixExpression &expr) {
    auto right = eval(expr.right);
    if (expr.op == "!") {
        if (std::holds_alternative<bool>(right)) {
            return !std::get<bool>(right);
        }
        if (std::holds_alternative<int64_t>(right)) {
            return std::get<int64_t>(right) == 0;
        }
        if (std::holds_alternative<std::nullptr_t>(right)) {
            return true;
        }
        return false;
    }
    if (expr.op == "-") {
        if (std::holds_alternative<int64_t>(right)) {
            return -std::get<int64_t>(right);
        }
    }
    return nullptr;
}

} // namespace

namespace monkey {

Object eval(const Program &program) {
    Object result;
    for (const auto &statement : program.statements) {
        result = eval(statement);
    }
    return result;
}

Object eval(const Statement &statement) {
    return std::visit(overloaded{[](const ExpressionStatement &stmt) -> Object {
                                     return eval(stmt.expression);
                                 },
                                 [](const auto &) -> Object { return Object{}; }},
                      statement);
}

Object eval(const Expression &expression) {
    return std::visit(
        overloaded{[](const IntegerLiteral &expr) -> Object { return expr.value; },
                   [](const BooleanLiteral &expr) -> Object { return expr.value; },
                   [](const Box<PrefixExpression> &expr) -> Object {
                       return evalPrefixExpression(*expr);
                   },
                   [](const auto &) -> Object { return Object{}; }},
        expression);
}

} // namespace monkey