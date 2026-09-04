#pragma once
#include "VIR/Function.h"
#include <unordered_map>
#include <vector>
namespace vir::analysis
{
class CFG
{
public:
explicit CFG(const Function &function);
const std::vector<BasicBlock *>& successors(const BasicBlock &)const;
const std::vector<BasicBlock *>& predecessors(const BasicBlock &)const;
const std::vector<BasicBlock *>& blocks()const
{
    return blocks_;
}
private:
std::vector<BasicBlock *> blocks_;
std::unordered_map<const BasicBlock *, std::vector<BasicBlock *> > successors_, predecessors_;
};
}
