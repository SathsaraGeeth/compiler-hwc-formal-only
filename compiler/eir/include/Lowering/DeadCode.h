/*
 * compiler/eir/include/Lowering/DeadCode.h
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

namespace emul::eir::lowering {
void eliminate_dead_code(Module& module);
}
