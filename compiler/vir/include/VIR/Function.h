#pragma once

#include "VIR/Region.h"
#include <string>

namespace vir
{
class Function
{
public:
Function(std::string name, Type type);
const std::string& name() const
{
    return name_;
}
const Type& type() const
{
    return type_;
}
Region& body()
{
    return body_;
}
const Region& body() const
{
    return body_;
}
BasicBlock& entry_block();
bool external() const
{
    return body_.blocks().empty();
}
private:
std::string name_;
Type type_;
Region body_;
};
} // namespace vir
