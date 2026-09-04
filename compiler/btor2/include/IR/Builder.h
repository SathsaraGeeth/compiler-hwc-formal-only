/*
 * compiler/btor2/include/IR/Builder.h
 *
 * Copyright (C) 2026 Sathsara Geeth
 */

/*
 * Version 1.0
 *
 * Version History
 *
 * Version | Description
 * --------+-----------------------------------------
 * 1.0     | Initial implementation
 */

/*
 * Comments:
 * 1. BTOR2 IR construction API.
 * 2. e.g., Builder builder;
 *          auto a = builder.input(8, "a");
 *          auto b = builder.input(8, "b");
 *          auto sum = builder.binary("add", a, b);
 * 3. Attrs
 *    - module_; owns operations, states, and formal properties
 * 4. Methods
 *    - input; creates a named input operation
 *    - state; creates a named state operation
 *    - init; sets the initial expression of a state
 *    - constant; creates a bit vector constant
 *    - unary; creates a one operand operation
 *    - binary; creates a two operand operation
 *    - ternary; creates a three operand operation such as ite
 *    - slice; extracts a bit range from a value
 *    - extend; zero extends or sign extends a value
 *    - next; sets the next expression of a state
 *    - output; exposes a value in the transition system interface
 *    - constraint; adds an environment constraint
 *    - bad; adds a safety failure condition
 *    - fair; adds a recurring fairness condition
 *    - justice; adds Büchi acceptance conditions
 *    - module; returns the constructed in memory BTOR2 IR
 *    - write; verifies and prints the constructed module
 * 5. Operation validates instruction semantics and Module validates
 *    state/property semantics; Builder only coordinates construction.
 */

#pragma once
#include "Module.h"
#include "Value.h"
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace emul::btor2 {
using Btor2Value = Value;

class Builder {
public:
    Btor2Value input(uint32_t width, std::string_view name);
    Btor2Value state(uint32_t width, std::string_view name);
    void init(Btor2Value state, Btor2Value value);
    Btor2Value constant(uint32_t width, std::string_view bits);
    Btor2Value unary(
        std::string_view opcode,
        Btor2Value value,
        uint32_t result_width = 0);
    Btor2Value binary(
        std::string_view opcode,
        Btor2Value left,
        Btor2Value right,
        uint32_t result_width = 0);
    Btor2Value ternary(
        std::string_view opcode,
        Btor2Value condition,
        Btor2Value left,
        Btor2Value right);
    Btor2Value slice(Btor2Value value, uint32_t offset, uint32_t width);
    Btor2Value extend(
        std::string_view opcode,
        Btor2Value value,
        uint32_t width);
    void next(Btor2Value state, Btor2Value value);
    void output(Btor2Value value, std::string_view name = {});
    void constraint(Btor2Value value, std::string_view name = {});
    void bad(Btor2Value value, std::string_view name = {});
    void fair(Btor2Value value, std::string_view name = {});
    void justice(
        const std::vector<Btor2Value>& conditions,
        std::string_view name = {});
    Module& module() noexcept;
    const Module& module() const noexcept;
    void write(std::ostream& output) const;

private:
    Module module_;

    Btor2Value add(Operation operation);
};

using Btor2Builder = Builder;
} // namespace emul::btor2
