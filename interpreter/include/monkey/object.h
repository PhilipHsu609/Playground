#pragma once

#include "monkey/ast.h"
#include "monkey/box.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace monkey {

class Environment;
struct ReturnValue;
struct Function;

using Error = std::string;
using Object =
    std::variant<int64_t, bool, std::nullptr_t, Box<ReturnValue>, Box<Function>, Error>;

struct ReturnValue {
    Object value;
};

struct Function {
    std::vector<Identifier> parameters;
    BlockStatement body;
    std::shared_ptr<Environment> env;
};

std::string inspect(const Object &obj);

} // namespace monkey