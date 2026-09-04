#pragma once
#include <string>
#include <vector>
namespace vir
{
class Module;
struct Diagnostic {
    std::string message;
};
class Verifier
{
public: static std::vector<Diagnostic> verify(const Module &);
};
}
