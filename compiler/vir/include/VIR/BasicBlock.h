#pragma once

#include "VIR/Operation.h"
#include <memory>
#include <string>
#include <vector>

namespace vir
{
class Region;

class BasicBlock
{
public:
explicit BasicBlock(std::string name = {});
const std::string& name() const
{
    return name_;
}
void set_name(std::string name)
{
    name_ = std::move(name);
}
Region * parent() const
{
    return parent_;
}
Value& add_argument(Type type, std::string name = {});
Operation& append(std::unique_ptr<Operation> operation);
std::unique_ptr<Operation> erase(Operation &operation);
const std::vector<std::unique_ptr<Value> >& arguments() const
{
    return arguments_;
}
const std::vector<std::unique_ptr<Operation> >& operations() const
{
    return operations_;
}
Operation * terminator() const;

private:
friend class Region;
std::string name_;
Region *parent_ = nullptr;
std::vector<std::unique_ptr<Value> > arguments_;
std::vector<std::unique_ptr<Operation> > operations_;
};
} // namespace vir
