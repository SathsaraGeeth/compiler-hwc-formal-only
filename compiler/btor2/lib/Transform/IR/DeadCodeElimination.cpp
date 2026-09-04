/*
 * compiler/btor2/lib/Transform/IR/DeadCodeElimination.cpp
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

#include "Transform/IR/DeadCodeElimination.h"
#include "Transform/IR/PassSupport.h"
#include <functional>

namespace emul::btor2::transform {
bool eliminate_dead_code(Module& module) {
    std::vector<bool> live(module.operations.size() + 1);
    std::function<void(NodeId)> mark = [&](NodeId id) {
        if (!id || live.at(id)) return;
        live[id] = true;
        const auto& operation = module.get(id);
        for (auto operand : operation.operands) mark(operand);
        if (operation.opcode == "state")
            for (const auto& state : module.states)
                if (state.value == id) {
                    if (state.initial) mark(*state.initial);
                    if (state.next) mark(*state.next);
                    break;
                }
    };
    for (const auto& output : module.outputs) mark(output.value);
    for (const auto& property : module.properties) {
        mark(property.condition);
        for (auto condition : property.conditions) mark(condition);
    }
    const auto before = module.operations.size();
    retain(module, live);
    return module.operations.size() != before;
}
}
