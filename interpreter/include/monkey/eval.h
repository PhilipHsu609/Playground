#pragma once

#include "monkey/ast.h"
#include "monkey/env.h"
#include "monkey/object.h"

namespace monkey {

Object eval(const Program &program, Environment &env);
Object eval(const Statement &statement, Environment &env);
Object eval(const Expression &expression, Environment &env);

} // namespace monkey