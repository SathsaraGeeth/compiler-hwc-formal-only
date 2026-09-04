/*
 * compiler/btor2/lib/Transform/IR/CommonSubexpressionElimination.cpp
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

#include "Transform/IR/CommonSubexpressionElimination.h"
#include "Transform/IR/PassSupport.h"
#include <algorithm>
#include <map>
#include <tuple>

namespace emul::btor2::transform {
bool eliminate_common_subexpressions(Module& module) {
    using Key = std::tuple<std::string, uint32_t, std::vector<NodeId>,
                           std::vector<uint32_t>, std::string>;
    std::map<Key, NodeId> values;
    std::vector<NodeId> replacement(module.operations.size() + 1);
    for (NodeId id = 1; id <= module.operations.size(); ++id) {
        replacement[id] = id;
        auto operation = module.get(id);
        for (auto& operand : operation.operands)
            operand = resolve(operand, replacement);
        if (operation.opcode == "input" || operation.opcode == "state") continue;
        if ((operation.opcode == "and" || operation.opcode == "or" ||
             operation.opcode == "xor" || operation.opcode == "add" ||
             operation.opcode == "mul" || operation.opcode == "eq" ||
             operation.opcode == "neq") && operation.operands.size() == 2 &&
            operation.operands[1] < operation.operands[0])
            std::swap(operation.operands[0], operation.operands[1]);
        Key key{operation.opcode, operation.width, operation.operands,
                operation.immediates, operation.bits};
        auto [found, inserted] = values.emplace(std::move(key), id);
        if (!inserted) replacement[id] = found->second;
    }
    return apply_replacements(module, replacement);
}
}
