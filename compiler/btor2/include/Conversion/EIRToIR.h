/*
 * compiler/btor2/include/Conversion/EIRToIR.h
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
 * Converts an EIR program into BTOR2 IR
 */

#pragma once
#include "IR/Module.h"
namespace emul::eir {
struct Program;
}
namespace emul::btor2 {
Module lower(const eir::Program& program);
}
