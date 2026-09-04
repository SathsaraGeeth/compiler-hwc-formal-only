#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace emul::formal {
enum class Result {
    proved, bounded, covered, counterexample, unknown, timeout, error
};

class Solver {
public:
    Solver();
    ~Solver();
    Solver(Solver&&) noexcept;
    Solver& operator=(Solver&&) noexcept;
    Solver(const Solver&) = delete;
    Solver& operator=(const Solver&) = delete;

    using BitVector = uint32_t;
    using Formula = uint32_t;

    BitVector variable(std::string_view name, uint32_t width);
    BitVector constant(uint64_t value, uint32_t width);
    BitVector zero_extend(BitVector value, uint32_t width);
    BitVector add(BitVector left, BitVector right);
    BitVector concat(BitVector high, BitVector low);
    BitVector slice(BitVector value, uint32_t offset, uint32_t width);
    Formula equal(BitVector left, BitVector right);
    Result prove(Formula property);
    std::string counterexample() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> implementation_;
};
} // namespace emul::formal
