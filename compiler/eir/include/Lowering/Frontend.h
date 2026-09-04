/*
 * compiler/eir/include/Lowering/Frontend.h
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

#pragma once
#include "../IR/Module.h"
#include "frontend/elaborated_design.h"
namespace emul::lowering {
eir::Program to_eir(const frontend::ElaboratedDesign& design);
eir::Module to_eir(frontend::SemanticNode body);
}
