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
struct Builtin;
struct Array;

struct Error {
    std::string message;
};

struct String {
    std::string value;
};

using Object = std::variant<int64_t, bool, std::nullptr_t, String, Box<ReturnValue>,
                            Box<Function>, Box<Builtin>, Box<Array>, Error>;

struct ReturnValue {
    Object value;
};

struct Function {
    std::vector<Identifier> parameters;
    BlockStatement body;
    std::shared_ptr<Environment> env;
};

using BuiltinFunction = Object (*)(const std::vector<Object> &args);

struct Builtin {
    BuiltinFunction fn;
};

struct Array {
    std::vector<Object> elements;
};

std::string inspect(const Object &obj);

} // namespace monkey