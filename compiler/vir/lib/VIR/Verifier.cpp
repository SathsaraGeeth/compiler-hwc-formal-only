#include "VIR/Verifier.h"
#include "VIR/Module.h"
#include <unordered_set>
namespace vir
{
std::vector<Diagnostic> Verifier::verify(const Module &module)
{
    std::vector<Diagnostic> out;
    std::unordered_set<std::string> functions;
    for (auto &fn:module.functions())
    {
        if (!functions.insert(fn->name()).second)
            out.push_back({"duplicate function @" + fn->name()});
        std::unordered_set<std::string> blocks;
        for (auto &block:fn->body().blocks())
        {
            if (!blocks.insert(block->name()).second)
                out.push_back({"duplicate block ^" + block->name() + " in @" + fn->name()});
            if (!block->terminator())
                out.push_back({"block ^" + block->name() + " has no terminator"});
            for (auto &op:block->operations())
                for (auto *operand:op->operands())
                    if (!operand)
                        out.push_back({"null operand in " + op->name()});
        }
    }
    return out;
}
} // namespace vir
