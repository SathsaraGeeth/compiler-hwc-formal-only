/*
 * compiler/btor2/lib/Transform/IR/AlgebraicSimplification.cpp
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

#include "Transform/IR/AlgebraicSimplification.h"
#include "Transform/IR/PassSupport.h"

namespace emul::btor2::transform {
namespace {
bool is_bits(const Module& module, NodeId id, char bit) {
    const auto& operation = module.get(id);
    return operation.opcode == "const" &&
           operation.bits.find_first_not_of(bit) == std::string::npos;
}
bool is_one(const Module& module, NodeId id) {
    const auto& operation = module.get(id);
    return operation.opcode == "const" && !operation.bits.empty() &&
           operation.bits.back() == '1' &&
           operation.bits.substr(0, operation.bits.size() - 1)
                   .find_first_not_of('0') == std::string::npos;
}
}

bool simplify_algebra(Module& module) {
    std::vector<NodeId> replacement(module.operations.size() + 1);
    for (NodeId id = 1; id <= module.operations.size(); ++id) {
        replacement[id] = id;
        const auto& operation = module.get(id);
        if (operation.operands.size() != 2) continue;
        auto left = resolve(operation.operands[0], replacement);
        auto right = resolve(operation.operands[1], replacement);
        if (operation.opcode == "and" && is_bits(module, right, '1'))
            replacement[id] = left;
        else if (operation.opcode == "and" && is_bits(module, left, '1'))
            replacement[id] = right;
        else if ((operation.opcode == "or" || operation.opcode == "xor" ||
                  operation.opcode == "add") && is_bits(module, right, '0'))
            replacement[id] = left;
        else if ((operation.opcode == "or" || operation.opcode == "xor" ||
                  operation.opcode == "add") && is_bits(module, left, '0'))
            replacement[id] = right;
        else if (operation.opcode == "sub" && is_bits(module, right, '0'))
            replacement[id] = left;
        else if (operation.opcode == "mul" && is_one(module, right))
            replacement[id] = left;
        else if (operation.opcode == "mul" && is_one(module, left))
            replacement[id] = right;
        else if ((operation.opcode == "and" || operation.opcode == "mul") &&
                 is_bits(module, right, '0'))
            replacement[id] = right;
        else if ((operation.opcode == "and" || operation.opcode == "mul") &&
                 is_bits(module, left, '0'))
            replacement[id] = left;
        else if ((operation.opcode == "eq" || operation.opcode == "neq") &&
                 left == right)
            for (NodeId prior = 1; prior < id; ++prior)
                if (module.get(prior).opcode == "const" &&
                    module.get(prior).width == 1 &&
                    module.get(prior).bits ==
                        (operation.opcode == "eq" ? "1" : "0")) {
                    replacement[id] = prior;
                    break;
                }
    }
    return apply_replacements(module, replacement);
}
}
