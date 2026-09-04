/*
 * compiler/eir/include/Lowering/LowerToMIR.h
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
#include "../MIR/MachineModule.h"
namespace emul::eir::lowering {
mir::MachineModule lower_to_mir(const Program& program);
}
