/*
 * compiler/target/include/target/emul/disassembler.h
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
 * 1. Declares disassembly of emulator machine programs.
 */

#pragma once
#include "MIR/MachineProgram.h"
#include <iosfwd>

namespace emul::machine {
void disassemble(const Program& program, std::ostream& output);
}
