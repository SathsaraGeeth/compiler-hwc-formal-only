#include "IRGen/SymbolTable.h"
#include <stdexcept>
namespace vir::irgen
{
void SymbolTable::push_scope()
{
    values_.emplace_back();
}
void SymbolTable::pop_scope()
{
    if (values_.size() == 1)
        throw std::logic_error("cannot pop root scope");
    values_.pop_back();
}
void SymbolTable::bind(std::string name, Value &value)
{
    if (!values_.back().emplace(std::move(name), &value).second)
        throw std::invalid_argument("duplicate value symbol");
}
Value * SymbolTable::lookup_value(const std::string &name)const
{
    for (auto it = values_.rbegin(); it != values_.rend(); ++it)
    {
        auto found = it->find(name);
        if (found != it->end())
            return found->second;
    }
    return nullptr;
}
void SymbolTable::bind(std::string name, Function &fn)
{
    if (!functions_.emplace(std::move(name), &fn).second)
        throw std::invalid_argument("duplicate function symbol");
}
Function * SymbolTable::lookup_function(const std::string &name)const
{
    auto it = functions_.find(name);
    return it == functions_.end()?nullptr:it->second;
}
} // namespace vir::irgen
