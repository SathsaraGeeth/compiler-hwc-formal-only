#pragma once
#include <stdexcept>
#include <string>

namespace vir::runtime::builtin {
class RuntimeError : public std::runtime_error { using std::runtime_error::runtime_error; };
class Error {
public:
    [[noreturn]] static void raise(std::string message) { throw RuntimeError(std::move(message)); }
    template<class Function, class Handler> static void catch_error(Function&& body, Handler&& handler) {
        try { body(); } catch (const RuntimeError& error) { handler(error); }
    }
    [[noreturn]] static void terminate(std::string message);
};
}
