/*
 * compiler/eir/lib/Lowering/Frontend/Lower.cpp
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

#include "../../../include/Lowering/Frontend.h"
#include "module.h"

namespace emul::lowering {
eir::Program to_eir(const frontend::ElaboratedDesign& design) {
    return semantic::lower_design(design);
}

eir::Module to_eir(frontend::SemanticNode body) {
    return semantic::lower_module(body);
}
}
