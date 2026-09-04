/*
 * compiler/btor2/include/HIR/Printer.h
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
 * Prints the higher level BTOR2 property representation
 */

#pragma once
#include "Module.h"
#include <iosfwd>

namespace emul::btor2::hir {
void print(const Module& module, std::ostream& output);
}
