/*
 * compiler/eir/include/MIR/MachineFunction.h
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
 * 1. Machine function - one lowered hardware entry point.
 * 2. e.g., machine @adder(%a : 4s<8>, %b : 4s<8>)
 *                  -> (%sum : 4s<8>)
 * 3. Attrs
 *    - name
 *    - inputs
 *    - results
 *    - states
 *    - blocks
 * 4. Methods
 *    - entry; returns the first machine block
 *    - find_state; resolves a declared state by name
 */

#pragma once
#include "MachineBlock.h"
#include "../IR/Module.h"
#include <string>
#include <string_view>
#include <vector>

namespace emul::mir {
struct MachineFunction {
    std::string name;
    std::vector<eir::Value> inputs;
    std::vector<eir::Value> results;
    std::vector<eir::State> states;
    std::vector<MachineBlock> blocks;
    MachineBlock* entry() noexcept;
    const MachineBlock* entry() const noexcept;
    const eir::State* find_state(std::string_view name) const noexcept;
};
}
