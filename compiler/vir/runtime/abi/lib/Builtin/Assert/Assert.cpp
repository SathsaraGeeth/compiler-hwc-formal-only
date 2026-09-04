#include "Builtin/Assert/Assert.h"

namespace vir::runtime::builtin {
void Assert::check(bool condition, std::string message) { if (!condition) fail(std::move(message)); }
[[noreturn]] void Assert::fail(std::string message) { throw AssertionFailure(std::move(message)); }
}
