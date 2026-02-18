#include "monkey/ast.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <ranges>
#include <string>
#include <variant>

namespace monkey {

std::string tokenLiteral(const Program &program) {
    if (program.statements.empty()) {
        return "";
    }
    // Return the token literal of the first statement for simplicity
    return monkey::tokenLiteral(program.statements[0]);
}

std::string toString(const Program &program) {
    std::string result;
    for (const auto &stmt : program.statements) {
        result += toString(stmt);
    }
    return result;
}

std::string toString(const Expression &expr) {
    return std::visit(
        overloaded{
            [](const Identifier &s) { return tokenLiteral(s); },
            [](const IntegerLiteral &s) { return tokenLiteral(s); },
            [](const BooleanLiteral &s) { return tokenLiteral(s); },
            [](const Box<PrefixExpression> &s) {
                return fmt::format("({}{})", s->op, toString(s->right));
            },
            [](const Box<InfixExpression> &s) {
                return fmt::format("({} {} {})", toString(s->left), s->op,
                                   toString(s->right));
            },
            [](const Box<IfExpression> &s) {
                std::string result = fmt::format("if {} {}", toString(s->condition),
                                                 toString(s->consequence));
                if (s->alternative) {
                    result += fmt::format(" else {}", toString(*s->alternative));
                }
                return result;
            },
            [](const Box<FunctionLiteral> &s) {
                return fmt::format(
                    "fn({}) {}",
                    fmt::join(std::views::transform(
                                  s->parameters,
                                  [](const Identifier &p) { return toString(p); }),
                              ", "),
                    toString(s->body));
            },
        },
        expr);
}

std::string toString(const Statement &stmt) {
    return std::visit(
        overloaded{
            [](const LetStatement &s) {
                return fmt::format("{} {} = {};", tokenLiteral(s), tokenLiteral(s.name),
                                   toString(s.value));
            },
            [](const ReturnStatement &s) {
                return fmt::format("{} {};", tokenLiteral(s), toString(s.value));
            },
            [](const ExpressionStatement &s) { return toString(s.expression); },
            [](const BlockStatement &s) {
                std::string result = "{ ";
                for (const auto &stmt : s.statements) {
                    result += toString(stmt);
                }
                result += " }";
                return result;
            },
        },
        stmt);
}

} // namespace monkey