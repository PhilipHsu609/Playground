#pragma once

#include "monkey/ast.h"
#include "monkey/env.h"
#include "monkey/object.h"

#include <memory>

namespace monkey {

Object eval(const Program &program, const std::shared_ptr<Environment> &env);
Object eval(const Statement &statement, const std::shared_ptr<Environment> &env);
Object eval(const Expression &expression, const std::shared_ptr<Environment> &env);

} // namespace monkey