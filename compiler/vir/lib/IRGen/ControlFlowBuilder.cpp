#include "IRGen/ControlFlowBuilder.h"
namespace vir::irgen
{
Operation& ControlFlowBuilder::branch(BasicBlock &target, std::vector<Value *> args)
{
    return builder_.core().create_branch(target, std::move(args));
}
Operation& ControlFlowBuilder::conditional(Value &condition, BasicBlock &yes, BasicBlock &no)
{
    return builder_.core().create_cond_branch(condition, yes, no);
}
Operation& ControlFlowBuilder::return_value(Value *value)
{
    return builder_.core().create_return(value);
}
} // namespace vir::irgen
