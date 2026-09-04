/*
 * compiler/btor2/lib/Transform/IR/ConstantFolding.cpp
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

#include "Transform/IR/ConstantFolding.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>

namespace emul::btor2::transform {
namespace {
using boost::multiprecision::cpp_int;

cpp_int value(const Operation& operation) {
    cpp_int result = 0;
    for (char bit : operation.bits) result = (result << 1) | (bit == '1');
    return result;
}

cpp_int mask(uint32_t width) { return (cpp_int(1) << width) - 1; }

cpp_int signed_value(cpp_int value, uint32_t width) {
    return (value & (cpp_int(1) << (width - 1))) ?
        value - (cpp_int(1) << width) : value;
}

std::string bits(cpp_int value, uint32_t width) {
    value &= mask(width);
    std::string result(width, '0');
    for (uint32_t bit = 0; bit < width; ++bit)
        if ((value >> bit) & 1) result[width - bit - 1] = '1';
    return result;
}
}

bool fold_constants(Module& module) {
    bool changed = false;
    for (NodeId id = 1; id <= module.operations.size(); ++id) {
        auto& operation = module.operations[id - 1];
        if (operation.opcode == "input" || operation.opcode == "state" ||
            operation.opcode == "const")
            continue;
        std::vector<const Operation*> operands;
        bool constant = true;
        for (auto operand : operation.operands) {
            const auto& source = module.get(operand);
            constant &= source.opcode == "const";
            operands.push_back(&source);
        }
        if (!constant) continue;
        const auto opcode = operation.opcode;
        cpp_int result = 0;
        bool valid = true;
        if (opcode == "not") result = ~value(*operands[0]);
        else if (opcode == "redand")
            result = value(*operands[0]) == mask(operands[0]->width);
        else if (opcode == "redor") result = value(*operands[0]) != 0;
        else if (opcode == "redxor") {
            auto number = value(*operands[0]);
            bool parity = false;
            while (number) { parity = !parity; number &= number - 1; }
            result = parity;
        } else if (opcode == "slice") {
            result = value(*operands[0]) >> operation.immediates[1];
        } else if (opcode == "uext") result = value(*operands[0]);
        else if (opcode == "sext")
            result = signed_value(value(*operands[0]), operands[0]->width);
        else if (opcode == "ite")
            result = value(*operands[value(*operands[0]) != 0 ? 1 : 2]);
        else if (operands.size() == 2) {
            auto left = value(*operands[0]);
            auto right = value(*operands[1]);
            if (opcode == "and") result = left & right;
            else if (opcode == "or") result = left | right;
            else if (opcode == "xor") result = left ^ right;
            else if (opcode == "add") result = left + right;
            else if (opcode == "sub") result = left - right;
            else if (opcode == "mul") result = left * right;
            else if (opcode == "udiv") { if (right == 0) valid = false; else result = left / right; }
            else if (opcode == "urem") { if (right == 0) valid = false; else result = left % right; }
            else if (opcode == "eq") result = left == right;
            else if (opcode == "neq") result = left != right;
            else if (opcode == "ult") result = left < right;
            else if (opcode == "ulte") result = left <= right;
            else if (opcode == "ugt") result = left > right;
            else if (opcode == "ugte") result = left >= right;
            else if (opcode == "slt") result = signed_value(left, operands[0]->width) < signed_value(right, operands[1]->width);
            else if (opcode == "slte") result = signed_value(left, operands[0]->width) <= signed_value(right, operands[1]->width);
            else if (opcode == "sgt") result = signed_value(left, operands[0]->width) > signed_value(right, operands[1]->width);
            else if (opcode == "sgte") result = signed_value(left, operands[0]->width) >= signed_value(right, operands[1]->width);
            else if (opcode == "concat") result = (left << operands[1]->width) | right;
            else if (opcode == "sll" || opcode == "srl" || opcode == "sra") {
                auto shift = right > operation.width ? operation.width : right.convert_to<uint32_t>();
                if (opcode == "sll") result = left << shift;
                else if (opcode == "srl") result = left >> shift;
                else result = signed_value(left, operands[0]->width) >> shift;
            } else valid = false;
        } else valid = false;
        if (!valid) continue;
        operation = Operation::constant(operation.width, bits(result, operation.width));
        changed = true;
    }
    return changed;
}
}
