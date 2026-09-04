/*
 * compiler/target/lib/cpu/cpu_target.cpp
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
 * 1. Implements lowering from EIR to the native CPU target.
 */

#include "target/cpu/cpu_target.h"
#include "Lowering/LowerToMIR.h"

namespace emul::target::cpu {
mir::MachineModule lower(const eir::Program& program) {
    /* Q: How is EIR prepared for native CPU execution? */
    /* A: Lower it to the same MIR */
    return eir::lowering::lower_to_mir(program);
}
}
