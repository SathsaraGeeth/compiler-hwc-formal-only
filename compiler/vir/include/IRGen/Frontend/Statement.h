#pragma once

#include "IRGen/Frontend/Expression.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace vir::irgen::frontend {
struct CoverageBinModel {
    std::string name;
    std::vector<emul::frontend::SemanticNode> values;
};
struct CoveragePointModel {
    emul::frontend::SemanticNode expression;
    std::vector<CoverageBinModel> bins;
};
using CoverageGroups = std::unordered_map<std::string,
    std::vector<CoveragePointModel>>;
class StatementLowerer {
public:
    explicit StatementLowerer(Context& context, std::string configured_test = {},
                              const std::unordered_set<std::string>* tasks = nullptr,
                              const CoverageGroups* coverage_groups = nullptr)
        : context_(context), expressions_(context, &locals_, &process_), configured_test_(std::move(configured_test)),
          tasks_(tasks), coverage_groups_(coverage_groups) {}
    void lower(emul::frontend::SemanticNode node);
    void begin_process();
    void finish_process();

private:
    Context& context_;
    std::unordered_set<std::string> locals_;
    bool process_ = false;
    ExpressionLowerer expressions_;
    std::string configured_test_;
    std::vector<BasicBlock*> break_targets_;
    std::vector<std::pair<std::uint64_t, BasicBlock*>> resume_states_;
    std::uint64_t next_resume_state_ = 1;
    const std::unordered_set<std::string>* tasks_ = nullptr;
    const CoverageGroups* coverage_groups_ = nullptr;
};
}
