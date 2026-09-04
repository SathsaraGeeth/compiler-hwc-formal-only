/*
 * compiler/btor2/lib/IR/Verifier.cpp
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

#include "../../include/IR/Verifier.h"
#include <stdexcept>
namespace emul::btor2 {
void verify(const Module& module) {
    for (NodeId id = 1; id <= module.operations.size(); ++id) {
        const auto& node = module.get(id);
        if (!node.width) throw std::runtime_error("zero-width BTOR2 node");
        for (auto operand : node.operands)
            if (!operand || operand >= id) throw std::runtime_error("non-dominating BTOR2 operand");
    }
    for (const auto& state : module.states) {
        const auto& declaration = module.get(state.value);
        if (declaration.opcode != "state")
            throw std::runtime_error("BTOR2 state does not reference a state node");
        if (state.initial && module.get(*state.initial).width != declaration.width)
            throw std::runtime_error("BTOR2 initial-state width mismatch");
        if (!state.next) throw std::runtime_error("BTOR2 state lacks next expression");
        if (module.get(*state.next).width != declaration.width)
            throw std::runtime_error("BTOR2 next-state width mismatch");
    }
    for (const auto& output : module.outputs)
        module.get(output.value);
    for (const auto& property : module.properties) {
        if (property.kind == PropertyKind::justice) {
            if (property.conditions.empty())
                throw std::runtime_error("BTOR2 justice property has no conditions");
            for (auto condition : property.conditions)
                if (module.get(condition).width != 1)
                    throw std::runtime_error("BTOR2 justice condition is not Boolean");
        } else if (module.get(property.condition).width != 1) {
            throw std::runtime_error("BTOR2 property condition is not Boolean");
        }
    }
}
}
