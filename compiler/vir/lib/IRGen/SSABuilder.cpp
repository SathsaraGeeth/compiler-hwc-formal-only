#include "IRGen/SSABuilder.h"
#include <stdexcept>
namespace vir::irgen
{
void SSABuilder::write(std::string name, BasicBlock &block, Value &value)
{
    states_[&block].values.insert_or_assign(std::move(name), &value);
}
void SSABuilder::add_predecessor(BasicBlock &block, BasicBlock &pred)
{
    auto &s = states_[&block];
    if (s.sealed)
        throw std::logic_error("predecessor added to sealed block");
    s.predecessors.push_back(&pred);
}
bool SSABuilder::sealed(const BasicBlock &block)const
{
    auto it = states_.find(const_cast<BasicBlock *>(&block));
    return it != states_.end() && it->second.sealed;
}
Value * SSABuilder::read(const std::string &name, BasicBlock &block, Type type)
{
    auto &s = states_[&block];
    auto it = s.values.find(name);
    return it == s.values.end()?read_recursive(name, block, std::move(type)):it->second;
}
Value * SSABuilder::read_recursive(const std::string &name, BasicBlock &block, Type type)
{
    auto &s = states_[&block];
    if (!s.sealed)
    {
        auto &arg = block.add_argument(std::move(type), name);
        s.incomplete[name] = &arg;
        s.values[name] = &arg;
        return &arg;
    }
    if (s.predecessors.empty())
        return nullptr;
    Value *first = read(name, *s.predecessors.front(), type);
    if (!first)
        return nullptr;
    for (std::size_t i = 1; i < s.predecessors.size(); ++i)
        if (read(name, *s.predecessors[i], type) != first)
        {
            auto &arg = block.add_argument(std::move(type), name);
            s.values[name] = &arg;
            return &arg;
        }
    s.values[name] = first;
    return first;
}
void SSABuilder::seal(BasicBlock &block)
{
    auto &s = states_[&block];
    s.sealed = true;
    auto pending = std::move(s.incomplete);
    for (auto & [name, value]:pending)
    {
        (void)value;
        s.values.erase(name);
        read_recursive(name, block, value->type());
    }
}
} // namespace vir::irgen
