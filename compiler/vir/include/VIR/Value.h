#pragma once

#include "VIR/Type.h"
#include <cstddef>
#include <string>
#include <vector>

namespace vir
{
class Operation;
class BasicBlock;

class Value
{
public:
enum class Origin {
    OperationResult, BlockArgument
};
struct Use {
    Operation *user;
    std::size_t operand;
};

const Type& type() const
{
    return type_;
}
const std::string& name() const
{
    return name_;
}
void set_name(std::string name)
{
    name_ = std::move(name);
}
Origin origin() const
{
    return origin_;
}
Operation * defining_operation() const
{
    return operation_;
}
BasicBlock * owning_block() const
{
    return block_;
}
std::size_t index() const
{
    return index_;
}
const std::vector<Use>& uses() const
{
    return uses_;
}

private:
friend class Operation;
friend class BasicBlock;
Value(Type type, Operation *op, std::size_t index);
Value(Type type, BasicBlock *block, std::size_t index);
void add_use(Operation *user, std::size_t operand);
void remove_use(Operation *user, std::size_t operand);
Type type_;
std::string name_;
Origin origin_;
Operation *operation_ = nullptr;
BasicBlock *block_ = nullptr;
std::size_t index_ = 0;
std::vector<Use> uses_;
};
} // namespace vir
