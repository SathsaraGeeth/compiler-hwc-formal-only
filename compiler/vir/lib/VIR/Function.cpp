#include "VIR/Function.h"
#include <stdexcept>
namespace vir
{
Function::Function(std::string name, Type type) : name_(std::move(name)), type_(std::move(type)),
    body_(nullptr)
{
    if (type_.kind() != Type::Kind::Function)
        throw std::invalid_argument("function requires function type");
}
BasicBlock& Function::entry_block()
{
    return body_.blocks().empty()?body_.add_block("entry"):*body_.blocks().front();
}
}
