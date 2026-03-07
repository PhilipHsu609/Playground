#include "monkey/eval.h"
#include "monkey/ast.h"
#include "monkey/box.h"
#include "monkey/env.h"
#include "monkey/object.h"
#include "monkey/overload.h"

#include <fmt/format.h>

#include <cstdint>
#include <memory>
#include <ranges>
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

Object evalProgram(const std::vector<Statement> &statements,
                   const std::shared_ptr<Environment> &env) {
    Object result;
    for (const auto &statement : statements) {
        result = eval(statement, env);
        if (std::holds_alternative<Box<ReturnValue>>(result)) {
            return std::get<Box<ReturnValue>>(result)->value;
        }
        if (std::holds_alternative<Error>(result)) {
            return std::get<Error>(result);
        }
    }
    return result;
}

Object evalBlockStatements(const std::vector<Statement> &statements,
                           const std::shared_ptr<Environment> &env) {
    Object result;
    for (const auto &statement : statements) {
        result = eval(statement, env);
        if (std::holds_alternative<Box<ReturnValue>>(result)) {
            return result;
        }
        if (std::holds_alternative<Error>(result)) {
            return result;
        }
    }
    return result;
}

Object evalLetStatement(const LetStatement &stmt,
                        const std::shared_ptr<Environment> &env) {
    auto value = eval(stmt.value, env);
    if (std::holds_alternative<Error>(value)) {
        return value;
    }
    env->set(tokenLiteral(stmt.name), value);
    return nullptr;
}

std::vector<Object> evalExpressions(const std::vector<Expression> &exps,
                                    const std::shared_ptr<Environment> &env) {
    std::vector<Object> result;
    for (const auto &e : exps) {
        auto evaluated = eval(e, env);
        if (std::holds_alternative<Error>(evaluated)) {
            return {evaluated};
        }
        result.push_back(evaluated);
    }
    return result;
}

Object evalPrefixExpression(const PrefixExpression &expr,
                            const std::shared_ptr<Environment> &env) {
    auto right = eval(expr.right, env);

    if (std::holds_alternative<Error>(right)) {
        return right;
    }
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
    return Error{
        fmt::format("unknown operator: {}{}", expr.op, tokenLiteral(expr.right))};
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
    return Error{fmt::format("unknown operator: {} {} {}", left, op, right)};
}

Object evalBooleanInfixExpression(const std::string &op, bool left, bool right) {
    if (op == "==") {
        return left == right;
    }
    if (op == "!=") {
        return left != right;
    }
    return Error{fmt::format("unknown operator: {} {} {}", left, op, right)};
}

Object evalInfixExpression(const InfixExpression &expr,
                           const std::shared_ptr<Environment> &env) {
    auto left = eval(expr.left, env);
    if (std::holds_alternative<Error>(left)) {
        return left;
    }

    auto right = eval(expr.right, env);
    if (std::holds_alternative<Error>(right)) {
        return right;
    }

    if (std::holds_alternative<int64_t>(left) && std::holds_alternative<int64_t>(right)) {
        int64_t leftVal = std::get<int64_t>(left);
        int64_t rightVal = std::get<int64_t>(right);
        return evalIntegerInfixExpression(expr.op, leftVal, rightVal);
    }
    if (std::holds_alternative<bool>(left) && std::holds_alternative<bool>(right)) {
        bool leftVal = std::get<bool>(left);
        bool rightVal = std::get<bool>(right);
        return evalBooleanInfixExpression(expr.op, leftVal, rightVal);
    }
    if (std::holds_alternative<String>(left) && std::holds_alternative<String>(right)) {
        const auto &leftVal = std::get<String>(left).value;
        const auto &rightVal = std::get<String>(right).value;
        if (expr.op == "+") {
            return String{leftVal + rightVal};
        }
        return Error{
            fmt::format("unknown operator: {} {} {}", leftVal, expr.op, rightVal)};
    }

    return Error{fmt::format("type mismatch: {} {} {}", tokenLiteral(expr.left), expr.op,
                             tokenLiteral(expr.right))};
}

Object evalIfExpression(const IfExpression &expr,
                        const std::shared_ptr<Environment> &env) {
    auto condition = eval(expr.condition, env);
    if (std::holds_alternative<Error>(condition)) {
        return condition;
    }

    if (isTrue(condition)) {
        return eval(expr.consequence, env);
    }

    if (expr.alternative.has_value()) {
        return eval(*expr.alternative, env);
    }

    return nullptr;
}

Object evalIdentifier(const Identifier &expr, const std::shared_ptr<Environment> &env) {
    auto value = env->get(tokenLiteral(expr));
    if (value.has_value()) {
        return *value;
    }
    return Error{fmt::format("identifier not found: {}", tokenLiteral(expr))};
}

Object evalCallExpression(const CallExpression &expr,
                          const std::shared_ptr<Environment> &env) {
    // Evaluate the function whether it's a function literal or an identifier
    auto function = eval(expr.function, env);
    if (std::holds_alternative<Error>(function)) {
        return function;
    }

    // Evaluate the arguments
    auto args = evalExpressions(expr.arguments, env);
    if (!args.empty() && std::holds_alternative<Error>(args[0])) {
        return args[0];
    }

    if (!std::holds_alternative<Box<Function>>(function)) {
        return Error{fmt::format("not a function: {}", inspect(function))};
    }

    // Extend the function's environment with the arguments from the ouside
    auto fn = std::get<Box<Function>>(function);
    auto extendedEnv = std::make_shared<Environment>(fn->env);
    for (const auto &[param, arg] : std::views::zip(fn->parameters, args)) {
        extendedEnv->set(tokenLiteral(param), arg);
    }

    // Evaluate the function body in the extended environment
    auto evaluated = eval(fn->body, extendedEnv);

    // Unwrap the return value if it's a ReturnValue, otherwise return the evaluated
    // result
    if (std::holds_alternative<Box<ReturnValue>>(evaluated)) {
        return std::get<Box<ReturnValue>>(evaluated)->value;
    }
    return evaluated;
}

} // namespace

namespace monkey {

Object eval(const Program &program, const std::shared_ptr<Environment> &env) {
    return evalProgram(program.statements, env);
}

Object eval(const Statement &statement, const std::shared_ptr<Environment> &env) {
    return std::visit(overloaded{[&env](const ExpressionStatement &stmt) -> Object {
                                     return eval(stmt.expression, env);
                                 },
                                 [&env](const BlockStatement &stmt) -> Object {
                                     return evalBlockStatements(stmt.statements, env);
                                 },
                                 [&env](const ReturnStatement &stmt) -> Object {
                                     auto result = eval(stmt.value, env);
                                     if (std::holds_alternative<Error>(result)) {
                                         return result;
                                     }
                                     return ReturnValue{result};
                                 },
                                 [&env](const LetStatement &stmt) -> Object {
                                     return evalLetStatement(stmt, env);
                                 },
                                 [](const auto &) -> Object { return Object{}; }},
                      statement);
}

Object eval(const Expression &expression, const std::shared_ptr<Environment> &env) {
    return std::visit(
        overloaded{
            [](const IntegerLiteral &expr) -> Object { return expr.value; },
            [](const BooleanLiteral &expr) -> Object { return expr.value; },
            [](const StringLiteral &expr) -> Object { return String{expr.value}; },
            [&env](const Identifier &expr) -> Object {
                return evalIdentifier(expr, env);
            },
            [&env](const Box<PrefixExpression> &expr) -> Object {
                return evalPrefixExpression(*expr, env);
            },
            [&env](const Box<InfixExpression> &expr) -> Object {
                return evalInfixExpression(*expr, env);
            },
            [&env](const Box<IfExpression> &expr) -> Object {
                return evalIfExpression(*expr, env);
            },
            [&env](const Box<FunctionLiteral> &expr) -> Object {
                return Function{expr->parameters, expr->body, env};
            },
            [&env](const Box<CallExpression> &expr) -> Object {
                return evalCallExpression(*expr, env);
            },
            [](const auto &) -> Object { return Error{"unknown expression type"}; }},
        expression);
}

} // namespace monkey