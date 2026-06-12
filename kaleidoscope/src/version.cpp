#include "kaleidoscope/version.hpp"

#include <llvm/Config/llvm-config.h>

namespace kaleidoscope {

std::string_view version() { return "kaleidoscope 0.1.0 (LLVM " LLVM_VERSION_STRING ")"; }

} // namespace kaleidoscope
