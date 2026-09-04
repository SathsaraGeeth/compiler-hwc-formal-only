/*
 * compiler/eir/include/MIR/MachineModule.h
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
 * 0. Machine module owns the complete MIR program.
 * 1. e.g., MachineModule containing machine @adder.
 * 2. Attrs
 *    - functions; lowered machine entry points
 * 3. Methods
 *    - root; returns the program entry machine function
 */

#pragma once
#include "MachineFunction.h"
#include <vector>

namespace emul::mir {
struct MachineModule {
    std::vector<MachineFunction> functions;
    MachineFunction& root();
    const MachineFunction& root() const;
};
}
