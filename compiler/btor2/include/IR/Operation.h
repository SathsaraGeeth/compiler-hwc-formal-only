/*
 * compiler/btor2/include/IR/Operation.h
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
 * 1. BTOR2 instruction or value-producing operation.
 * 2. e.g., Operation{"add", 8, {2, 4}}
 * 3. Attrs
 *    - opcode; BTOR2 operation name such as add, state, or const
 *    - width; bit-vector result width
 *    - operands; IDs of operations consumed by this operation
 *    - immediates; literal arguments such as slice upper/lower indices
 *    - symbol; optional input, state, or operation name
 *    - bits; binary payload used by const
 * 4. Operator Classes
 *    - unary; one input value such as not
 *    - binary; two input values such as add or eq
 *    - ternary; three input values such as ite
 *    - extension; one value and an extension amount
 * 5. Methods
 *    - is_operator; checks that an opcode belongs to an operator class
 *    - input; creates and validates an input operation
 *    - state; creates and validates a state operation
 *    - constant; validates and creates a binary constant
 *    - unary; validates and creates a unary operation
 *    - binary; validates and creates a binary operation
 *    - ternary; validates condition and data widths
 *    - slice; validates and creates a bit slice
 *    - extend; validates and creates an extension
 * 6. Operation operands refer only to earlier operations in the module.
 */

#pragma once
#include "Value.h"
#include <string>
#include <string_view>
#include <vector>

namespace emul::btor2 {
enum class OperatorClass {
    unary,
    binary,
    ternary,
    extension
};

struct Operation {
    std::string opcode;
    std::uint32_t width = 0;
    std::vector<NodeId> operands;
    std::vector<std::uint32_t> immediates;
    std::string symbol;
    std::string bits;

    static Operation input(std::uint32_t width, std::string_view name);
    static Operation state(std::uint32_t width, std::string_view name);
    static Operation constant(std::uint32_t width, std::string_view bits);
    static Operation unary(
        std::string_view opcode,
        Value value,
        std::uint32_t result_width = 0);
    static Operation binary(
        std::string_view opcode,
        Value left,
        Value right,
        std::uint32_t result_width = 0);
    static Operation ternary(
        std::string_view opcode,
        Value condition,
        Value left,
        Value right);
    static Operation slice(Value value, std::uint32_t offset,
                           std::uint32_t width);
    static Operation extend(std::string_view opcode, Value value,
                            std::uint32_t width);
};

bool is_operator(OperatorClass operator_class, std::string_view opcode);
} // namespace emul::btor2
