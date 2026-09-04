#include "VIR/Builder.h"
#include <stdexcept>
namespace vir
{
Function& Builder::create_function(std::string name, Type type, bool declaration)
{
    auto &fn = module_.add_function(std::move(name), std::move(type));
    if (!declaration)
        block_ = &fn.entry_block();
    return fn;
}
BasicBlock& Builder::create_block(Region &region, std::string name)
{
    return region.add_block(std::move(name));
}
Operation& Builder::create(std::string name, std::vector<Value *> operands, std::vector<Type>
                           results, std::map<std::string, Attribute> attrs)
{
    if (!block_)
        throw std::logic_error("no insertion block");
    return block_->append(std::make_unique<Operation>(std::move(name), std::move(operands), std::
                                                      move(results), std::move(attrs)));
}
Operation& Builder::create_constant(Type type, Attribute value)
{
    return create("const", {}, {std::move(type)}, {{"value", std::move(value)}});
}
Operation& Builder::create_return(Value *value)
{
    return create("ret", value?std::vector<Value *>{value}:std::vector<Value *>{});
}
Operation& Builder::create_branch(BasicBlock &target, std::vector<Value *> args)
{
    return create("br", std::move(args), {}, {{"target", target.name()}});
}
Operation& Builder::create_cond_branch(Value &condition, BasicBlock &yes, BasicBlock &no)
{
    return create("cond_br", {&condition}, {}, {{"true", yes.name()}, {"false", no.name()}});
}
} // namespace vir
