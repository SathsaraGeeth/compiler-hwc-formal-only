/*
 * compiler/target/include/target/cpu/cpu_target.h
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
 * 1. Declares the CPU target
 */

#pragma once

#include "IR/Module.h"
#include "MIR/MachineModule.h"

namespace emul::target::cpu {
mir::MachineModule lower(const eir::Program& program);
}
