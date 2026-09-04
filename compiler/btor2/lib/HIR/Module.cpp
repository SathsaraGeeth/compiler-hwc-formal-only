/*
 * compiler/btor2/lib/HIR/Module.cpp
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
 */

#include "HIR/Module.h"
#include <utility>

namespace emul::btor2::hir {
void Module::add(Operation operation) {
    operations.push_back(std::move(operation));
}

} 
