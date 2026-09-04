#include "VIR/Value.h"
#include <algorithm>
namespace vir
{
Value::Value(Type type, Operation *op, std::size_t index) : type_(std::move(type)), origin_(Origin::
                                                                                            OperationResult),
    operation_(op), index_(index)
{
}
Value::Value(Type type, BasicBlock *block, std::size_t index) : type_(std::move(type)), origin_(
        Origin::BlockArgument), block_(block), index_(index)
{
}
void Value::add_use(Operation *user, std::size_t operand)
{
    uses_.push_back({user, operand});
}
void Value::remove_use(Operation *user, std::size_t operand)
{
    std::erase_if(uses_, [&](const Use &use){
            return use.user == user && use.operand == operand;
        });
}
} // namespace vir
