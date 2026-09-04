/*
 * compiler/btor2/lib/Transform/IR/StrengthReduction.cpp
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
 *
 */

#include "Transform/IR/StrengthReduction.h"
#include <algorithm>

namespace emul::btor2::transform {
bool reduce_strength(Module& module) {
    bool changed = false;
    std::vector<uint32_t> uses(module.operations.size() + 1);
    for (const auto& operation : module.operations)
        for (auto operand : operation.operands) ++uses.at(operand);
    for (const auto& state : module.states) {
        if (state.initial) ++uses.at(*state.initial);
        if (state.next) ++uses.at(*state.next);
    }
    for (const auto& output : module.outputs) ++uses.at(output.value);
    for (const auto& property : module.properties) {
        if (property.condition) ++uses.at(property.condition);
        for (auto condition : property.conditions) ++uses.at(condition);
    }
    for (auto& operation : module.operations) {
        if ((operation.opcode != "mul" && operation.opcode != "udiv") ||
            operation.operands.size() != 2)
            continue;
        auto constant_operand = operation.operands[1];
        auto value_operand = operation.operands[0];
        if (operation.opcode == "mul" &&
            module.get(operation.operands[0]).opcode == "const") {
            constant_operand = operation.operands[0];
            value_operand = operation.operands[1];
        }
        const auto& constant = module.get(constant_operand);
        if (constant.opcode != "const" ||
            std::count(constant.bits.begin(), constant.bits.end(), '1') != 1 ||
            uses.at(constant_operand) != 1)
            continue;
        const auto shift = static_cast<uint32_t>(
            constant.bits.size() - constant.bits.rfind('1') - 1);
        
        operation.opcode = operation.opcode == "mul" ? "sll" : "srl";
        operation.operands = {value_operand, constant_operand};
        std::string bits(constant.width, '0');
        for (uint32_t value = shift, bit = 0; value; value >>= 1, ++bit)
            if (value & 1) bits[bits.size() - bit - 1] = '1';
        auto& mutable_constant = module.operations[constant_operand - 1];
        mutable_constant.bits = std::move(bits);
        changed = true;
    }
    return changed;
}
}
