/*
 * compiler/btor2/lib/Transform/IR/CopyPropagation.cpp
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

#include "Transform/IR/CopyPropagation.h"
#include "Transform/IR/PassSupport.h"

namespace emul::btor2::transform {
bool propagate_copies(Module& module) {
    std::vector<NodeId> replacement(module.operations.size() + 1);
    for (NodeId id = 1; id <= module.operations.size(); ++id) {
        replacement[id] = id;
        const auto& operation = module.get(id);
        if (operation.opcode == "ite" && operation.operands[1] == operation.operands[2])
            replacement[id] = resolve(operation.operands[1], replacement);
        else if (operation.opcode == "slice" && operation.immediates.size() == 2 &&
                 operation.immediates[1] == 0 &&
                 operation.immediates[0] + 1 == module.get(operation.operands[0]).width)
            replacement[id] = resolve(operation.operands[0], replacement);
        else if ((operation.opcode == "uext" || operation.opcode == "sext") &&
                 !operation.immediates.empty() && operation.immediates[0] == 0)
            replacement[id] = resolve(operation.operands[0], replacement);
    }
    return apply_replacements(module, replacement);
}
}
