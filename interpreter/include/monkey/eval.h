#pragma once

#include "monkey/ast.h"
#include "monkey/object.h"

namespace monkey {

Object eval(const Program &program);
Object eval(const Statement &statement);
Object eval(const Expression &expression);

} // namespace monkey