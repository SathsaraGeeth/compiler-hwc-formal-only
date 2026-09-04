#include "Builtin/Error/Error.h"
#include <cstdlib>
#include <iostream>

namespace vir::runtime::builtin {
[[noreturn]] void Error::terminate(std::string message) { std::cerr << message << '\n'; std::terminate(); }
}
