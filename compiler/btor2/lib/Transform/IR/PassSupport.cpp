/*
 * compiler/btor2/lib/Transform/IR/PassSupport.cpp
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

#include "Transform/IR/PassSupport.h"
#include <stdexcept>

namespace emul::btor2::transform {
NodeId resolve(NodeId value, const std::vector<NodeId>& replacements) {
    while (value && value < replacements.size() &&
           replacements[value] != value)
        value = replacements[value];
    return value;
}

namespace {
void rebuild(Module& module, const std::vector<bool>* live,
             const std::vector<NodeId>* replacements) {
    std::vector<NodeId> mapping(module.operations.size() + 1);
    std::vector<Operation> operations;
    operations.reserve(module.operations.size());
    for (NodeId old = 1; old <= module.operations.size(); ++old) {
        auto canonical = replacements ? resolve(old, *replacements) : old;
        if (canonical != old) continue;
        if (live && !live->at(old)) continue;
        auto operation = module.get(old);
        for (auto& operand : operation.operands) {
            if (replacements) operand = resolve(operand, *replacements);
            if (!operand || !mapping[operand])
                throw std::runtime_error("BTOR2 pass produced a forward reference");
            operand = mapping[operand];
        }
        operations.push_back(std::move(operation));
        mapping[old] = static_cast<NodeId>(operations.size());
    }
    if (replacements)
        for (NodeId old = 1; old < mapping.size(); ++old)
            if (!mapping[old]) mapping[old] = mapping[resolve(old, *replacements)];

    std::vector<State> states;
    for (auto state : module.states) {
        if (live && !live->at(state.value)) continue;
        state.value = mapping.at(state.value);
        if (state.initial) state.initial = mapping.at(*state.initial);
        if (state.next) state.next = mapping.at(*state.next);
        states.push_back(state);
    }
    for (auto& output : module.outputs) output.value = mapping.at(output.value);
    for (auto& property : module.properties) {
        if (property.condition) property.condition = mapping.at(property.condition);
        for (auto& condition : property.conditions)
            condition = mapping.at(condition);
    }
    module.operations = std::move(operations);
    module.states = std::move(states);
}
}

bool apply_replacements(Module& module,
                        const std::vector<NodeId>& replacements) {
    bool changed = false;
    for (NodeId id = 1; id < replacements.size(); ++id)
        changed |= replacements[id] != id;
    if (changed) rebuild(module, nullptr, &replacements);
    return changed;
}

void retain(Module& module, const std::vector<bool>& live) {
    rebuild(module, &live, nullptr);
}
}
