#pragma once
#include <stdexcept>
#include <string>

namespace vir::runtime::builtin {
class AssertionFailure : public std::runtime_error { using std::runtime_error::runtime_error; };
class Assert {
public:
    static void check(bool condition, std::string message = "assertion failed");
    [[noreturn]] static void fail(std::string message);
};
}
