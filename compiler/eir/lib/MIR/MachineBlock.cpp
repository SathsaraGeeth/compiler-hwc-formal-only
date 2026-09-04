/*
 * compiler/eir/lib/MIR/MachineBlock.cpp
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

#include "../../include/MIR/MachineBlock.h"

namespace emul::mir {
MachineInstr* MachineBlock::terminator() noexcept {
    if (instructions.empty() ||
        instructions.back().opcode != MachineOpcode::yield)
        return nullptr;
    return &instructions.back();
}

const MachineInstr* MachineBlock::terminator() const noexcept {
    if (instructions.empty() ||
        instructions.back().opcode != MachineOpcode::yield)
        return nullptr;
    return &instructions.back();
}
}
