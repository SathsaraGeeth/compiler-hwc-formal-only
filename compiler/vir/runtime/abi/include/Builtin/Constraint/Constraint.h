#pragma once
#include <cstddef>
#include <functional>
#include <vector>

namespace vir::runtime::builtin {
class Constraint {
public:
    using Expression = std::function<bool()>;
    void add(Expression expression);
    bool solve(size_t attempts = 1) const;
private:
    std::vector<Expression> expressions_;
};
}
