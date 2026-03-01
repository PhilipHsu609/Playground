#include "monkey/eval.h"
#include "monkey/ast.h"
#include "monkey/object.h"
#include "monkey/overload.h"

namespace monkey {

Object eval(const Program &program) {
    Object result;
    for (const auto &statement : program.statements) {
        result = eval(statement);
    }
    return result;
}

Object eval(const Statement &statement) {
    return std::visit(
        overloaded{[](const ExpressionStatement &stmt) -> Object {
                       return eval(stmt.expression);
                   },
                   []([[maybe_unused]] const auto &stmt) -> Object { return Object{}; }},
        statement);
}

Object eval(const Expression &expression) {
    return std::visit(
        overloaded{[](const IntegerLiteral &expr) -> Object { return expr.value; },
                   [](const BooleanLiteral &expr) -> Object { return expr.value; },
                   []([[maybe_unused]] const auto &expr) -> Object { return Object{}; }},
        expression);
}

} // namespace monkey