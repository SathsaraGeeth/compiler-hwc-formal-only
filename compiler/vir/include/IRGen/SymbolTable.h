#pragma once
#include <string>
#include <unordered_map>
#include <vector>
namespace vir
{
class Value;
class Function;
namespace irgen
{
class SymbolTable
{
public:
void push_scope();
void pop_scope();
void bind(std::string name, Value &value);
Value * lookup_value(const std::string &name) const;
void bind(std::string name, Function &function);
Function * lookup_function(const std::string &name) const;
private:
std::vector<std::unordered_map<std::string, Value *> > values_{{}};
std::unordered_map<std::string, Function *> functions_;
};
}
}
