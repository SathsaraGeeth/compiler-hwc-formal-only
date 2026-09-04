#include "VIR/Region.h"
namespace vir
{
BasicBlock& Region::add_block(std::string name)
{
    auto block = std::make_unique<BasicBlock>(std::move(name));
    block->parent_ = this;
    blocks_.push_back(std::move(block));
    return *blocks_.back();
}
}
