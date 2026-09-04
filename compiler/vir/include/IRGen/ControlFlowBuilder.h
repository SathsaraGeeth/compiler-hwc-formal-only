#pragma once
#include "IRGen/IRBuilder.h"
namespace vir::irgen
{
class ControlFlowBuilder
{
public:
explicit ControlFlowBuilder(IRBuilder &builder) : builder_(builder)
{
}
Operation& branch(BasicBlock &target, std::vector<Value *> args = {});
Operation& conditional(Value &condition, BasicBlock &yes, BasicBlock &no);
Operation& return_value(Value *value = nullptr);
private: IRBuilder &builder_;
};
}
