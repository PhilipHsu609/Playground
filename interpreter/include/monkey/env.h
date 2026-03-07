#pragma once

#include "monkey/object.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace monkey {

class Environment {
  public:
    explicit Environment(std::shared_ptr<Environment> outer = nullptr)
        : outer_(std::move(outer)) {}

    std::optional<Object> get(const std::string &name) const;
    void set(const std::string &name, Object value);

  private:
    std::unordered_map<std::string, Object> store_;
    std::shared_ptr<Environment> outer_;
};

std::shared_ptr<Environment> makeEnvironment();

} // namespace monkey