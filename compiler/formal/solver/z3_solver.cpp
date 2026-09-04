#include "solver.h"
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <z3++.h>

namespace emul::formal {
class Solver::Implementation {
public:
    z3::context context;
    z3::solver solver{context};
    std::vector<z3::expr> vectors;
    std::vector<z3::expr> formulas;
    std::string model;

    BitVector vector(z3::expr expression) {
        vectors.push_back(std::move(expression));
        return static_cast<BitVector>(vectors.size());
    }
    Formula formula(z3::expr expression) {
        formulas.push_back(std::move(expression));
        return static_cast<Formula>(formulas.size());
    }
    z3::expr& bv(BitVector id) {
        if (!id || id > vectors.size()) throw std::out_of_range("invalid formal bit-vector");
        return vectors[id - 1];
    }
    z3::expr& pred(Formula id) {
        if (!id || id > formulas.size()) throw std::out_of_range("invalid formal formula");
        return formulas[id - 1];
    }
};

Solver::Solver() : implementation_(std::make_unique<Implementation>()) {}
Solver::~Solver() = default;
Solver::Solver(Solver&&) noexcept = default;
Solver& Solver::operator=(Solver&&) noexcept = default;

Solver::BitVector Solver::variable(std::string_view name, uint32_t width) {
    if (!width) throw std::invalid_argument("zero-width formal variable");
    return implementation_->vector(implementation_->context.bv_const(
        std::string(name).c_str(), width));
}
Solver::BitVector Solver::constant(uint64_t value, uint32_t width) {
    if (!width) throw std::invalid_argument("zero-width formal constant");
    return implementation_->vector(implementation_->context.bv_val(value, width));
}
Solver::BitVector Solver::zero_extend(BitVector value, uint32_t width) {
    auto& input = implementation_->bv(value);
    auto current = input.get_sort().bv_size();
    if (width < current) throw std::invalid_argument("formal extension narrows value");
    return implementation_->vector(z3::zext(input, width - current));
}
Solver::BitVector Solver::add(BitVector left, BitVector right) {
    auto& lhs = implementation_->bv(left); auto& rhs = implementation_->bv(right);
    if (lhs.get_sort().bv_size() != rhs.get_sort().bv_size())
        throw std::invalid_argument("formal add width mismatch");
    return implementation_->vector(lhs + rhs);
}
Solver::BitVector Solver::concat(BitVector high, BitVector low) {
    return implementation_->vector(z3::concat(implementation_->bv(high),
                                               implementation_->bv(low)));
}
Solver::BitVector Solver::slice(BitVector value, uint32_t offset, uint32_t width) {
    auto& input = implementation_->bv(value);
    if (!width || uint64_t(offset) + width > input.get_sort().bv_size())
        throw std::out_of_range("formal slice is outside value");
    return implementation_->vector(input.extract(offset + width - 1, offset));
}
Solver::Formula Solver::equal(BitVector left, BitVector right) {
    return implementation_->formula(implementation_->bv(left) == implementation_->bv(right));
}
Result Solver::prove(Formula property) {
    implementation_->solver.push();
    implementation_->solver.add(!implementation_->pred(property));
    auto result = implementation_->solver.check();
    implementation_->model.clear();
    if (result == z3::sat) implementation_->model = implementation_->solver.get_model().to_string();
    implementation_->solver.pop();
    return result == z3::unsat ? Result::proved :
           result == z3::sat ? Result::counterexample : Result::unknown;
}
std::string Solver::counterexample() const { return implementation_->model; }
} // namespace emul::formal
