#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

namespace monkey {

using Object = std::variant<int64_t, bool, std::nullptr_t>;

std::string inspect(const Object &obj);

} // namespace monkey