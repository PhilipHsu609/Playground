#include "monkey/ast.h"

#include <string>

namespace monkey {

std::string Program::tokenLiteral() const {
    if (statements.empty()) {
        return "";
    }
    // Return the token literal of the first statement for simplicity
    return monkey::tokenLiteral(statements[0]);
}

} // namespace monkey