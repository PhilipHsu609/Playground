#include "monkey/ast.h"

#include <string>

namespace monkey {

std::string tokenLiteral(const Program &program) {
    if (program.statements.empty()) {
        return "";
    }
    // Return the token literal of the first statement for simplicity
    return monkey::tokenLiteral(program.statements[0]);
}

std::string toString(const Expression &expr) {
    return std::visit(overloaded{
                          [](const Identifier &s) { return tokenLiteral(s); },
                          [](const IntegerLiteral &s) { return tokenLiteral(s); },
                          [](const Box<PrefixExpression> &s) {
                              return fmt::format("({}{})", s->operator_,
                                                 toString(s->right));
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
        },
        stmt);
}

} // namespace monkey