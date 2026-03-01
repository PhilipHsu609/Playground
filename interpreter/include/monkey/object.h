#pragma once

#include "monkey/box.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

namespace monkey {

struct ReturnValue;

using Error = std::string;
using Object = std::variant<int64_t, bool, std::nullptr_t, Box<ReturnValue>, Error>;

struct ReturnValue {
    Object value;
};

std::string inspect(const Object &obj);

} // namespace monkey