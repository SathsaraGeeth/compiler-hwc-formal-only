/*
 * compiler/eir/include/IR/Printer.h
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
 * 1. Convert EIR back to textual representation
 * 2. Program is in memory EIR
 */


#pragma once
#include "Module.h"
#include <iosfwd>

namespace emul::eir { 
    void print(const Program& program, std::ostream& output); 
}
