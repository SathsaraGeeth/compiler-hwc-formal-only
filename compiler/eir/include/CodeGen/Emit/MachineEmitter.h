/*
 * compiler/eir/include/CodeGen/Emit/MachineEmitter.h
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
#include "../../MIR/MachineProgram.h"
#include "../../MIR/MachineModule.h"
#include "../../IR/Module.h"
#include "target/target.h"

namespace emul::machine {
Program encode(const eir::Program& program, const target::Target& target);
Program encode(const mir::MachineModule& module, const target::Target& target);
}
