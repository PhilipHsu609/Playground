#include "monkey/object.h"
#include "monkey/box.h"
#include "monkey/overload.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <ranges>
#include <string>
#include <variant>

namespace monkey {

std::string inspect(const Object &obj) {
    return std::visit(
        overloaded{
            [](int64_t value) { return std::to_string(value); },
            [](bool value) -> std::string { return value ? "true" : "false"; },
            [](const String &s) -> std::string { return s.value; },
            [](std::nullptr_t) -> std::string { return "null"; },
            [](const Box<ReturnValue> &rv) -> std::string { return inspect(rv->value); },
            [](const Box<Function> &fn) -> std::string {
                return fmt::format(
                    "fn({}) {}",
                    fmt::join(std::views::transform(
                                  fn->parameters,
                                  [](const Identifier &p) { return tokenLiteral(p); }),
                              ", "),
                    toString(fn->body));
            },
            [](const Error &err) -> std::string { return "ERROR: " + err.message; }},
        obj);
}

} // namespace monkey