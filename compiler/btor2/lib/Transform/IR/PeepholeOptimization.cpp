/*
 * compiler/btor2/lib/Transform/IR/PeepholeOptimization.cpp
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

#include "Transform/IR/PeepholeOptimization.h"
#include "Transform/IR/PassSupport.h"

namespace emul::btor2::transform {
bool optimize_peepholes(Module& module) {
    std::vector<NodeId> replacement(module.operations.size() + 1);
    for (NodeId id = 1; id <= module.operations.size(); ++id) {
        replacement[id] = id;
        const auto& operation = module.get(id);
        if (operation.opcode != "not" || operation.operands.size() != 1)
            continue;
        const auto& inner = module.get(resolve(operation.operands[0], replacement));
        if (inner.opcode == "not")
            replacement[id] = resolve(inner.operands[0], replacement);
    }
    return apply_replacements(module, replacement);
}
}
