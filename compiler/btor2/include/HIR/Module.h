/*
 * compiler/btor2/include/HIR/Module.h
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
 * Stores an ordered collection of higher level temporal property operations
 */

#pragma once
#include "Operation.h"
#include <vector>

namespace emul::btor2::hir {
struct Module {
    std::vector<Operation> operations;

    void add(Operation operation);
};

} 
