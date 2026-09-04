/*
 * compiler/eir/include/IR/Module.h
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
 * 0. Program has many modules; complete design
 *    - root is the top
 * 1. Module (like in SV)
 * 2. e.g. module @adder
 * 3. Attrs
 *    - inputs
 *    - outputs
 *    - states
 *    - operations
 * 4. Methods
 *    - find_inputs; e.g. auto clk = module.find_input("clk") => returns Value* for clk input
 *    - find_states
 */

#pragma once
#include "Operation.h"
#include "Value.h"
#include <string>
#include <string_view>
#include <vector>

namespace emul::eir {
struct State {
    std::string name;
    std::string type;
    Type value_type() const;
    friend bool operator==(const State&, const State&) = default;
};
struct Module {
    std::string name;
    std::vector<Value> inputs;
    std::vector<Value> results;
    std::vector<State> states;
    std::vector<Operation> operations;
    const Value* find_input(std::string_view name) const noexcept;
    const State* find_state(std::string_view name) const noexcept;
};
struct Program {
    std::vector<Module> modules;
    Module* root() noexcept;
    const Module* root() const noexcept;
    Module* find_module(std::string_view name) noexcept;
    const Module* find_module(std::string_view name) const noexcept;
};
}
