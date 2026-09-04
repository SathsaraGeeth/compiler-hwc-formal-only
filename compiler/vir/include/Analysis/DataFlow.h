#pragma once
#include "Analysis/CFG.h"
#include <unordered_map>
#include <unordered_set>
namespace vir::analysis
{
struct LiveValues {
    std::unordered_set<const Value *> in;
    std::unordered_set<const Value *> out;
};
class DataFlow
{
public:
explicit DataFlow(const CFG &cfg) : cfg_(cfg)
{
}
void run_liveness();
const LiveValues& liveness(const BasicBlock &block)const;
private: const CFG &cfg_;
std::unordered_map<const BasicBlock *, LiveValues> live_;
};
}
