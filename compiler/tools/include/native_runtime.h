/*
 * compiler/tools/include/native_runtime.h
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

#include "VIR/Module.h"
#include "eir/include/MIR/MachineProgram.h"
#include "runtime/transport/transport.h"
#include <string>

namespace emul::tool {

bool execute_native(const vir::Module& module, std::string entry,
                    const machine::Program& hardware,
                    std::string instance, transport::Transport& transport);

}
