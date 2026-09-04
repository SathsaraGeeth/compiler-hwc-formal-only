/*
 * compiler/btor2/lib/IR/Module.cpp
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

#include "../../include/IR/Module.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace emul::btor2 {
namespace {
std::string symbol(std::string_view name) {
    std::string result(name);
    std::replace_if(result.begin(), result.end(), [](char character) {
        return !(std::isalnum(static_cast<unsigned char>(character)) ||
                 character == '_' || character == '.');
    }, '_');
    return result;
}

void require_boolean(Value value, std::string_view kind) {
    if (value.width != 1)
        throw std::runtime_error("BTOR2 " + std::string(kind) +
                                 " is not Boolean");
}
}

NodeId Module::add(Operation operation) {
    if (!operation.width)
        throw std::runtime_error("zero-width BTOR2 operation");
    operations.push_back(std::move(operation));
    return static_cast<NodeId>(operations.size());
}

const Operation& Module::get(NodeId id) const {
    if (!id || id > operations.size())
        throw std::out_of_range("BTOR2 operation id");
    return operations[id - 1];
}

State& Module::find_state(Value value) {
    auto found = std::find_if(states.begin(), states.end(), [&](const State& state) {
        return state.value == value.node;
    });
    if (found == states.end())
        throw std::runtime_error("BTOR2 transition does not target a state");
    return *found;
}

void Module::add_state(Value state) {
    if (get(static_cast<NodeId>(state.node)).opcode != "state")
        throw std::runtime_error("BTOR2 state record requires a state operation");
    states.push_back({static_cast<NodeId>(state.node), {}, {}});
}

void Module::init(Value state, Value value) {
    if (state.width != value.width)
        throw std::runtime_error("BTOR2 initial-state width mismatch");
    find_state(state).initial = static_cast<NodeId>(value.node);
}

void Module::next(Value state, Value value) {
    if (state.width != value.width)
        throw std::runtime_error("BTOR2 next-state width mismatch");
    find_state(state).next = static_cast<NodeId>(value.node);
}

void Module::output(Value value, std::string_view name) {
    if (!value || value.node > operations.size())
        throw std::runtime_error("BTOR2 output references an invalid value");
    outputs.push_back({static_cast<NodeId>(value.node), symbol(name)});
}

void Module::constraint(Value condition, std::string_view name) {
    require_boolean(condition, "constraint");
    properties.push_back({PropertyKind::constraint,
        static_cast<NodeId>(condition.node), symbol(name), {}});
}

void Module::bad(Value condition, std::string_view name) {
    require_boolean(condition, "bad property");
    properties.push_back({PropertyKind::bad,
        static_cast<NodeId>(condition.node), symbol(name), {}});
}

void Module::fair(Value condition, std::string_view name) {
    require_boolean(condition, "fair condition");
    properties.push_back({PropertyKind::fair,
        static_cast<NodeId>(condition.node), symbol(name), {}});
}

void Module::justice(
    const std::vector<Value>& conditions,
    std::string_view name) {
    if (conditions.empty())
        throw std::runtime_error("BTOR2 justice property has no conditions");
    std::vector<NodeId> values;
    values.reserve(conditions.size());
    for (auto condition : conditions) {
        require_boolean(condition, "justice condition");
        values.push_back(static_cast<NodeId>(condition.node));
    }
    properties.push_back(
        {PropertyKind::justice, 0, symbol(name), std::move(values)});
}
} 
