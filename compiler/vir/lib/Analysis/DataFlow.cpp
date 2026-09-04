#include "Analysis/DataFlow.h"
namespace vir::analysis
{
void DataFlow::run_liveness()
{
    for (auto *b:cfg_.blocks())
        live_[b] = {}
    ;
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (auto it = cfg_.blocks().rbegin(); it != cfg_.blocks().rend(); ++it)
        {
            auto *b = *it;
            std::unordered_set<const Value *> out;
            for (auto *succ:cfg_.successors(*b))
                out.insert(live_[succ].in.begin(), live_[succ].in.end());
            auto in = out;
            for (auto op = b->operations().rbegin(); op != b->operations().rend(); ++op)
            {
                for (std::size_t i = 0; i < (*op)->result_count(); ++i)
                    in.erase(&(*op)->result(i));
                for (auto *operand:(*op)->operands())
                    in.insert(operand);
            }
            for (auto &arg:b->arguments())
                in.erase(arg.get());
            if (in != live_[b].in || out != live_[b].out)
            {
                live_[b] = {std::move(in), std::move(out)};
                changed = true;
            }
        }
    }
}
const LiveValues& DataFlow::liveness(const BasicBlock &b)const
{
    static const LiveValues empty;
    auto it = live_.find(&b);
    return it == live_.end()?empty:it->second;
}
} // namespace vir::analysis
