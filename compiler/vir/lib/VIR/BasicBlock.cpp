#include "VIR/BasicBlock.h"
#include <algorithm>
#include <stdexcept>
namespace vir
{
BasicBlock::BasicBlock(std::string name) : name_(std::move(name))
{
}
Value& BasicBlock::add_argument(Type type, std::string name)
{
    auto value = std::unique_ptr<Value>(new Value(std::move(type), this, arguments_.size()));
    value->set_name(std::move(name));
    arguments_.push_back(std::move(value));
    return *arguments_.back();
}
Operation& BasicBlock::append(std::unique_ptr<Operation> operation)
{
    if (!operation)
        throw std::invalid_argument("null operation");
    operation->set_parent(this);
    operations_.push_back(std::move(operation));
    return *operations_.back();
}
std::unique_ptr<Operation> BasicBlock::erase(Operation &operation)
{
    auto it = std::find_if(operations_.begin(), operations_.end(), [&](auto &p){
            return p.get() == &operation;
        });
    if (it == operations_.end())
        return {}
    ;
    auto out = std::move(*it);
    operations_.erase(it);
    out->set_parent(nullptr);
    return out;
}
Operation * BasicBlock::terminator() const
{
    return !operations_.empty() && operations_.back()->is_terminator() ? operations_.back().get() :
           nullptr;
}
} // namespace vir
