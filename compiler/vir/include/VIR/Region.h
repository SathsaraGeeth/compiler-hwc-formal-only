#pragma once

#include "VIR/BasicBlock.h"
#include <memory>
#include <vector>

namespace vir
{
class Operation;

class Region
{
public:
explicit Region(Operation *parent = nullptr) : parent_(parent)
{
}
BasicBlock& add_block(std::string name = {});
Operation * parent() const
{
    return parent_;
}
const std::vector<std::unique_ptr<BasicBlock> >& blocks() const
{
    return blocks_;
}
private:
Operation *parent_;
std::vector<std::unique_ptr<BasicBlock> > blocks_;
};
} // namespace vir
