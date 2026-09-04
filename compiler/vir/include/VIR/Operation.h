#pragma once

#include "VIR/Attribute.h"
#include "VIR/Value.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace vir
{
class BasicBlock;
class Region;

class Operation
{
public:
Operation(std::string name, std::vector<Value *> operands = {},
          std::vector<Type> results = {}, std::map<std::string, Attribute> attributes = {});
~Operation();
Operation(const Operation &) = delete;
Operation& operator=(const Operation &) = delete;

const std::string& name() const
{
    return name_;
}
void set_name(std::string name)
{
    name_ = std::move(name);
}
BasicBlock * parent() const
{
    return parent_;
}
const std::vector<Value *>& operands() const
{
    return operands_;
}
Value& result(std::size_t index = 0)
{
    return *results_.at(index);
}
const Value& result(std::size_t index = 0) const
{
    return *results_.at(index);
}
std::size_t result_count() const
{
    return results_.size();
}
const std::map<std::string, Attribute>& attributes() const
{
    return attributes_;
}
const Attribute * attribute(const std::string &name) const;
void set_attribute(std::string name, Attribute value);
void erase_attribute(const std::string &name)
{
    attributes_.erase(name);
}
void set_operand(std::size_t index, Value &value);
Region& add_region();
const std::vector<std::unique_ptr<Region> >& regions() const
{
    return regions_;
}
bool is_terminator() const;

private:
friend class BasicBlock;
void set_parent(BasicBlock *parent)
{
    parent_ = parent;
}
std::string name_;
BasicBlock *parent_ = nullptr;
std::vector<Value *> operands_;
std::vector<std::unique_ptr<Value> > results_;
std::map<std::string, Attribute> attributes_;
std::vector<std::unique_ptr<Region> > regions_;
};
} // namespace vir
