#pragma once

#include "monkey/object.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace monkey {

class Environment {
  public:
    std::optional<Object> get(const std::string &name) const;
    void set(const std::string &name, Object value);

  private:
    std::unordered_map<std::string, Object> store_;
};

} // namespace monkey