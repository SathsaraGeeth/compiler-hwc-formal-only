#pragma once
#include "Analysis/CFG.h"
#include <unordered_map>
#include <unordered_set>
namespace vir::analysis
{
class DominatorTree
{
public:
explicit DominatorTree(const CFG &cfg);
bool dominates(const BasicBlock &a, const BasicBlock &b)const;
BasicBlock * immediate_dominator(const BasicBlock &block)const;
private:
std::unordered_map<const BasicBlock *, std::unordered_set<const BasicBlock *> > dominators_;
std::unordered_map<const BasicBlock *, BasicBlock *> immediate_;
};
}
