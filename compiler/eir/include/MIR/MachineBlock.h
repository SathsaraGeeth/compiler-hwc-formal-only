/*
 * compiler/eir/include/MIR/MachineBlock.h
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
 * 1. Machine basic block - single entry ordered instruction sequence.
 * 2. e.g., ^entry:
 *      %0 : 4s<8> = add %a, %b
 *      yield %0
 * 3. Attrs
 *    - name; printed with the ^ prefix
 *    - instructions
 * 4. Methods
 *    - terminator; returns the final yield or nullptr
 */

#pragma once
#include "MachineInstr.h"
#include <string>
#include <vector>

namespace emul::mir {
struct MachineBlock {
    std::string name;
    std::vector<MachineInstr> instructions;
    MachineInstr* terminator() noexcept;
    const MachineInstr* terminator() const noexcept;
};
}
