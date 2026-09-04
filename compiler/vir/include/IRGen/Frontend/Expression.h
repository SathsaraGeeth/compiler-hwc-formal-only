#pragma once

#include "IRGen/Frontend/Context.h"
#include "frontend/semantic_tree.h"
#include <unordered_set>

namespace vir::irgen::frontend {
class ExpressionLowerer {
public:
    explicit ExpressionLowerer(Context& context,
        const std::unordered_set<std::string>* locals = nullptr,
        const bool* process = nullptr)
        : context_(context), locals_(locals), process_(process) {}
    Value& lower(emul::frontend::SemanticNode node);
    void store(emul::frontend::SemanticNode target, Value& value);

private:
    Context& context_;
    const std::unordered_set<std::string>* locals_ = nullptr;
    const bool* process_ = nullptr;
    bool local(emul::frontend::SemanticNode node) const;
};

std::string symbol_name(emul::frontend::SemanticNode node);
std::string constant_text(emul::frontend::SemanticNode node);
Type lower_type(emul::frontend::SemanticNode node);
std::int64_t parse_integer(std::string text);
}
