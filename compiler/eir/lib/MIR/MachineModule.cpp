/*
 * compiler/eir/lib/MIR/MachineModule.cpp
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

#include "../../include/MIR/MachineModule.h"
#include <stdexcept>

namespace emul::mir {
MachineFunction& MachineModule::root() {
    if (functions.empty())
        throw std::runtime_error("MIR module has no root function");
    return functions.back();
}

const MachineFunction& MachineModule::root() const {
    if (functions.empty())
        throw std::runtime_error("MIR module has no root function");
    return functions.back();
}
}
