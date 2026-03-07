#include "monkey/env.h"

#include <optional>
#include <string>

namespace monkey {

std::optional<Object> Environment::get(const std::string &name) const {
    auto it = store_.find(name);
    if (it != store_.end()) {
        return it->second;
    }
    if (outer_ != nullptr) {
        return outer_->get(name);
    }
    return std::nullopt;
}

void Environment::set(const std::string &name, Object value) {
    store_[name] = std::move(value);
}

std::shared_ptr<Environment> makeEnvironment() { return std::make_shared<Environment>(); }

} // namespace monkey