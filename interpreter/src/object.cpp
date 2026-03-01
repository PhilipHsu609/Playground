#include "monkey/object.h"
#include "monkey/overload.h"

#include <string>
#include <variant>

namespace monkey {

std::string inspect(const Object &obj) {
    return std::visit(
        overloaded{[](int64_t value) { return std::to_string(value); },
                   [](bool value) -> std::string { return value ? "true" : "false"; },
                   [](std::nullptr_t) -> std::string { return "null"; }},
        obj);
}

} // namespace monkey