/*
 * compiler/btor2/lib/Transform/IR/BranchSimplification.cpp
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

#include "Transform/IR/BranchSimplification.h"
#include "Transform/IR/PassSupport.h"

namespace emul::btor2::transform {
bool simplify_branches(Module& module) {
    std::vector<NodeId> replacement(module.operations.size() + 1);
    for (NodeId id = 1; id <= module.operations.size(); ++id) {
        replacement[id] = id;
        const auto& operation = module.get(id);
        if (operation.opcode != "ite" || operation.operands.size() != 3)
            continue;
        auto condition = resolve(operation.operands[0], replacement);
        auto yes = resolve(operation.operands[1], replacement);
        auto no = resolve(operation.operands[2], replacement);
        const auto& condition_operation = module.get(condition);
        if (yes == no)
            replacement[id] = yes;
        else if (condition_operation.opcode == "const")
            replacement[id] = condition_operation.bits.back() == '1' ? yes : no;
    }
    return apply_replacements(module, replacement);
}
}
