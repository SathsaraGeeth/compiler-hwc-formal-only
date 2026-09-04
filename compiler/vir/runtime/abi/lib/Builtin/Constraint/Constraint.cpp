#include "Builtin/Constraint/Constraint.h"

namespace vir::runtime::builtin {
void Constraint::add(Expression expression) { if (expression) expressions_.push_back(std::move(expression)); }
bool Constraint::solve(size_t attempts) const { while (attempts--) { bool valid = true; for (auto& expression : expressions_) valid &= expression(); if (valid) return true; } return false; }
}
