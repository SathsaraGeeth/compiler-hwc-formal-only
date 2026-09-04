/*
 * compiler/btor2/include/IR/Module.h
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
 * 1. Complete in-memory BTOR2 transition system.
 * 2. e.g., Module module;
 *          auto input = module.add({"input", 1, {}, {}, "enable"});
 * 3. State Attrs
 *    - value; ID of the state declaration operation
 *    - initial; optional initial-state expression ID
 *    - next; optional next-state expression ID
 * 4. Property Kinds
 *    - constraint; restricts legal executions
 *    - bad; describes a safety violation
 *    - cover; describes a reachability target
 *    - fair; requires a condition to recur on fair executions
 *    - justice; describes generalized Büchi acceptance conditions
 * 5. Property Attrs
 *    - kind; property classification
 *    - condition; Boolean condition for constraint, bad, cover, or fair
 *    - name; optional BTOR2 symbol
 *    - conditions; Boolean conditions for justice
 * 6. Module Attrs
 *    - operations; ordered BTOR2 operations
 *    - states; state transition relations
 *    - outputs; externally visible transition-system values
 *    - properties; formal objectives and constraints
 * 7. Methods
 *    - add; appends an operation and returns its stable one-based ID
 *    - get; returns an operation by ID and rejects an invalid ID
 *    - add_state; associates a State record with a state operation
 *    - init; validates and sets a state's initial expression
 *    - next; validates and sets a state's next expression
 *    - output; validates and adds an externally visible value
 *    - constraint; validates and adds an environment constraint
 *    - bad; validates and adds a safety property
 *    - fair; validates and adds a recurring fairness condition
 *    - justice; validates and adds Büchi acceptance conditions
 *    - find_state; finds the State record for a Value
 */

#pragma once
#include "Operation.h"
#include "Value.h"
#include <optional>
#include <string>
#include <vector>

namespace emul::btor2 {
struct State {
    NodeId value = 0;
    std::optional<NodeId> initial;
    std::optional<NodeId> next;
};

enum class PropertyKind { constraint, bad, cover, fair, justice };

struct Output {
    NodeId value;
    std::string name;
};

struct Property {
    PropertyKind kind;
    NodeId condition;
    std::string name;
    std::vector<NodeId> conditions;
};

struct Module {
    std::vector<Operation> operations;
    std::vector<State> states;
    std::vector<Output> outputs;
    std::vector<Property> properties;
    NodeId add(Operation operation);
    const Operation& get(NodeId id) const;
    void add_state(Value state);
    void init(Value state, Value value);
    void next(Value state, Value value);
    void output(Value value, std::string_view name = {});
    void constraint(Value condition, std::string_view name = {});
    void bad(Value condition, std::string_view name = {});
    void fair(Value condition, std::string_view name = {});
    void justice(const std::vector<Value>& conditions,
                 std::string_view name = {});

private:
    State& find_state(Value value);
};
} // namespace emul::btor2
