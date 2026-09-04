/*
 * compiler/eir/lib/IR/Module.cpp
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

namespace emul::eir {

Type State::value_type() const {
    return Type::parse(type);
}

const Value* Module::find_input(std::string_view name) const noexcept {
    auto found = std::find_if(inputs.begin(), inputs.end(),
              [&](const Value& input) { return input.name == name; });
    return found == inputs.end() ? nullptr : &*found;
}

const State* Module::find_state(std::string_view name) const noexcept {
    auto found = std::find_if(states.begin(), states.end(),
              [&](const State& state) { return state.name == name; });
    return found == states.end() ? nullptr : &*found;
}

Module* Program::root() noexcept {
    return modules.empty() ? nullptr : &modules.back();
}

const Module* Program::root() const noexcept {
    return modules.empty() ? nullptr : &modules.back();
}

Module* Program::find_module(std::string_view name) noexcept {
    auto found = std::find_if(modules.begin(), modules.end(),
            [&](const Module& module) { return module.name == name; });
    return found == modules.end() ? nullptr : &*found;
}

const Module* Program::find_module(std::string_view name) const noexcept {
    auto found = std::find_if(modules.begin(), modules.end(),
              [&](const Module& module) { return module.name == name; });
    return found == modules.end() ? nullptr : &*found;
}
}
