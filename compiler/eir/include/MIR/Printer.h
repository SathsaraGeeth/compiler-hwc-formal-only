/*
 * compiler/eir/include/MIR/Printer.h
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
 * 1. Convert the in memory MachineModule into textual MIR.
 * 2. Output is canonical mir assembly.
 * 3. The MIR parser accepts the emitted representation.
 */

#pragma once
#include "MachineModule.h"
#include <iosfwd>

namespace emul::mir {
    void print(const MachineModule& module, std::ostream& output);
}
