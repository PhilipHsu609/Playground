#include "monkey/eval.h"
#include "monkey/ast.h"
#include "monkey/box.h"
#include "monkey/object.h"
#include "monkey/overload.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace {

using namespace monkey;

bool isTrue(const Object &obj) {
    if (std::holds_alternative<bool>(obj)) {
        return std::get<bool>(obj);
    }
    if (std::holds_alternative<std::nullptr_t>(obj)) {
        return false;
    }
    return true;
}

Object evalProgram(const std::vector<Statement> &statements) {
    Object result;
    for (const auto &statement : statements) {
        result = eval(statement);
        if (std::holds_alternative<Box<ReturnValue>>(result)) {
            return std::get<Box<ReturnValue>>(result)->value;
        }
    }
    return result;
}

Object evalBlockStatements(const std::vector<Statement> &statements) {
    Object result;
    for (const auto &statement : statements) {
        result = eval(statement);
        if (std::holds_alternative<Box<ReturnValue>>(result)) {
            return result;
        }
    }
    return result;
}

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

Object evalIntegerInfixExpression(const std::string &op, int64_t left, int64_t right) {
    if (op == "+") {
        return left + right;
    }
    if (op == "-") {
        return left - right;
    }
    if (op == "*") {
        return left * right;
    }
    if (op == "/") {
        return left / right;
    }
    if (op == "<") {
        return left < right;
    }
    if (op == ">") {
        return left > right;
    }
    if (op == "==") {
        return left == right;
    }
    if (op == "!=") {
        return left != right;
    }
    throw std::runtime_error("unknown operator: " + op);
}

Object evalInfixExpression(const InfixExpression &expr) {
    auto left = eval(expr.left);
    auto right = eval(expr.right);

    if (std::holds_alternative<int64_t>(left) && std::holds_alternative<int64_t>(right)) {
        int64_t leftVal = std::get<int64_t>(left);
        int64_t rightVal = std::get<int64_t>(right);
        return evalIntegerInfixExpression(expr.op, leftVal, rightVal);
    }
    if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right)) {
        auto leftVal = static_cast<int64_t>(std::get<bool>(left));
        auto rightVal = static_cast<int64_t>(std::get<bool>(right));
        return evalIntegerInfixExpression(expr.op, leftVal, rightVal);
    }
    return nullptr;
}

Object evalIfExpression(const IfExpression &expr) {
    auto condition = eval(expr.condition);

    if (isTrue(condition)) {
        return eval(expr.consequence);
    }

    if (expr.alternative.has_value()) {
        return eval(*expr.alternative);
    }

    return nullptr;
}

} // namespace

namespace monkey {

Object eval(const Program &program) { return evalProgram(program.statements); }

Object eval(const Statement &statement) {
    return std::visit(overloaded{[](const ExpressionStatement &stmt) -> Object {
                                     return eval(stmt.expression);
                                 },
                                 [](const BlockStatement &stmt) -> Object {
                                     return evalBlockStatements(stmt.statements);
                                 },
                                 [](const ReturnStatement &stmt) -> Object {
                                     return ReturnValue{eval(stmt.value)};
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
                   [](const Box<InfixExpression> &expr) -> Object {
                       return evalInfixExpression(*expr);
                   },
                   [](const Box<IfExpression> &expr) -> Object {
                       return evalIfExpression(*expr);
                   },
                   [](const auto &) -> Object { return Object{}; }},
        expression);
}

} // namespace monkey