#include "Analysis/DominatorTree.h"
#include <algorithm>
namespace vir::analysis
{
DominatorTree::DominatorTree(const CFG &cfg)
{
    if (cfg.blocks().empty())
        return;
    std::unordered_set<const BasicBlock *> all(cfg.blocks().begin(), cfg.blocks().end());
    for (auto *b:cfg.blocks())
        dominators_[b] = (b == cfg.blocks().front()?std::unordered_set<const BasicBlock *>{b}:all);
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (std::size_t i = 1; i < cfg.blocks().size(); ++i)
        {
            auto *b = cfg.blocks()[i];
            auto preds = cfg.predecessors(*b);
            std::unordered_set<const BasicBlock *> next;
            if (!preds.empty())
            {
                next = dominators_[preds.front()];
                for (std::size_t p = 1; p < preds.size(); ++p)
                    for (auto it = next.begin(); it != next.end();)
                        if (!dominators_[preds[p]].contains(*it))
                            it = next.erase(it);
                        else
                            ++it;
            }
            next.insert(b);
            if (next != dominators_[b])
            {
                dominators_[b] = std::move(next);
                changed = true;
            }
        }
    }
    for (std::size_t i = 1; i < cfg.blocks().size(); ++i)
    {
        auto *b = cfg.blocks()[i];
        for (auto *candidate:dominators_[b])
        {
            if (candidate == b)
                continue;
            bool closest = true;
            for (auto *other:dominators_[b])
                if (other != b && other != candidate && dominators_[other].contains(candidate))
                {
                    closest = false;
                    break;
                }
            if (closest)
            {
                immediate_[b] = const_cast<BasicBlock *>(candidate);
                break;
            }
        }
    }
}
bool DominatorTree::dominates(const BasicBlock &a, const BasicBlock &b)const
{
    auto it = dominators_.find(&b);
    return it != dominators_.end() && it->second.contains(&a);
}
BasicBlock * DominatorTree::immediate_dominator(const BasicBlock &b)const
{
    auto it = immediate_.find(&b);
    return it == immediate_.end()?nullptr:it->second;
}
} // namespace vir::analysis
