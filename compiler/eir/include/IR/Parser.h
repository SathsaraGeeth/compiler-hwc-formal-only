/*
 * compiler/eir/include/IR/Parser.h
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
 * 1. Parse textual representation of EIR into in memory representation
 */

#pragma once
#include "Module.h"
#include <iosfwd>

namespace emul::eir {
    Program parse(std::istream& input);
}
