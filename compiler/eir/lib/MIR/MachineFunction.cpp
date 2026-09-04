/*
 * compiler/eir/lib/MIR/MachineFunction.cpp
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

#include "../../include/MIR/MachineFunction.h"
#include <algorithm>

namespace emul::mir {
MachineBlock* MachineFunction::entry() noexcept {
    return blocks.empty() ? nullptr : &blocks.front();
}

const MachineBlock* MachineFunction::entry() const noexcept {
    return blocks.empty() ? nullptr : &blocks.front();
}

const eir::State*
MachineFunction::find_state(std::string_view name) const noexcept {
    auto found = std::find_if(
        states.begin(), states.end(),
        [&](const eir::State& state) { return state.name == name; });
    return found == states.end() ? nullptr : &*found;
}
}
