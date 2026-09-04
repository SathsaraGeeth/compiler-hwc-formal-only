/*
 * compiler/btor2/include/IR/Printer.h
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
 * 1. Converts in memory BTOR2 IR into the standard textual representation.
 * 2. e.g., print(module, std::cout);
 * 3. The printer emits
 *    - shared bit vector sort declarations
 *    - ordered operations with remapped textual IDs
 *    - initial and next state relations
 *    - output declarations
 *    - constraint, bad, cover, fair, and justice properties
 * 4. Methods
 *    - print; serializes a Module to an output stream
 * 5. Printing does not mutate the module.
 */

#pragma once
#include "Module.h"
#include <iosfwd>
namespace emul::btor2 {
void print(const Module& module, std::ostream& output);
} // namespace emul::btor2
