#pragma once
#include <iosfwd>
#include <string>
namespace vir
{
class Module;
class Printer
{
public: static void print(const Module &, std::ostream &);
static std::string str(const Module &);
};
}
