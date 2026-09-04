#include "VIR/Module.h"
#include <stdexcept>
namespace vir
{
Function& Module::add_function(std::string name, Type type)
{
    if (symbols_.contains(name))
        throw std::invalid_argument("duplicate function: " + name);
    auto fn = std::make_unique<Function>(name, std::move(type));
    auto *ptr = fn.get();
    symbols_.emplace(std::move(name), ptr);
    functions_.push_back(std::move(fn));
    return *ptr;
}
Function * Module::find_function(const std::string &name) const
{
    auto it = symbols_.find(name);
    return it == symbols_.end()?nullptr:it->second;
}
void Module::set_attribute(std::string name, Attribute value)
{
    attributes_.insert_or_assign(std::move(name), std::move(value));
}
} // namespace vir
