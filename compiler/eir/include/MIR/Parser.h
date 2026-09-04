/*
 * compiler/eir/include/MIR/Parser.h
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
 * 1. Parse textual MIR into its in memory MachineModule.
 * 2. Input is mir  assembly.
 * 3. Output is syntax correct MIR; Verifier checks semantics.
 * 4. Syntax errors include the source line number.
 */

#pragma once
#include "MachineModule.h"
#include <iosfwd>

namespace emul::mir {
    MachineModule parse(std::istream& input);
}
