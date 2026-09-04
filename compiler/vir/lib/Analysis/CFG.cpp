#include "Analysis/CFG.h"
#include <string>
namespace vir::analysis
{
namespace
{
std::string text(const Attribute *attr)
{
    if (!attr)
        return {}
    ;
    if (auto *value = std::get_if<std::string>(&attr->value()))
        return *value;
    return {};
}
}
CFG::CFG(const Function &fn)
{
    std::unordered_map<std::string, BasicBlock *> names;
    for (auto &block:fn.body().blocks())
    {
        blocks_.push_back(block.get());
        names[block->name()] = block.get();
        successors_[block.get()] = {};
        predecessors_[block.get()] = {};
    }
    for (auto *block:blocks_)
    {
        auto *term = block->terminator();
        if (!term)
            continue;
        std::vector<std::string> targets;
        if (term->name() == "br")
            targets.push_back(text(term->attribute("target")));
        else if (term->name() == "cond_br")
        {
            targets.push_back(text(term->attribute("true")));
            targets.push_back(text(term->attribute("false")));
        }
        for (auto &target:targets)
        {
            auto it = names.find(target);
            if (it != names.end())
            {
                successors_[block].push_back(it->second);
                predecessors_[it->second].push_back(block);
            }
        }
    }
}
const std::vector<BasicBlock *>& CFG::successors(const BasicBlock &b)const
{
    static const std::vector<BasicBlock *> empty;
    auto it = successors_.find(&b);
    return it == successors_.end()?empty:it->second;
}
const std::vector<BasicBlock *>& CFG::predecessors(const BasicBlock &b)const
{
    static const std::vector<BasicBlock *> empty;
    auto it = predecessors_.find(&b);
    return it == predecessors_.end()?empty:it->second;
}
} // namespace vir::analysis
