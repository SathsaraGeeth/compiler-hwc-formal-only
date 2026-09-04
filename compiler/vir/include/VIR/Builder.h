#pragma once

#include "VIR/Module.h"

namespace vir
{
class Builder
{
public:
explicit Builder(Module &module) : module_(module)
{
}
void set_insertion_point(BasicBlock &block)
{
    block_ = &block;
}
Function& create_function(std::string name, Type type, bool declaration = false);
Module& module() const { return module_; }
BasicBlock& create_block(Region &region, std::string name = {});
Operation& create(std::string name, std::vector<Value *> operands = {},
                  std::vector<Type> results = {},
                  std::map<std::string, Attribute> attributes = {});
Operation& create_constant(Type type, Attribute value);
Operation& create_return(Value *value = nullptr);
Operation& create_branch(BasicBlock &target, std::vector<Value *> arguments = {});
Operation& create_cond_branch(Value &condition, BasicBlock &yes, BasicBlock &no);
private:
Module &module_;
BasicBlock *block_ = nullptr;
};
} // namespace vir
