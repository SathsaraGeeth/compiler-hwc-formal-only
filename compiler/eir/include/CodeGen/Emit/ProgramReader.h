/*
 * compiler/eir/include/CodeGen/Emit/ProgramReader.h
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
#include "../../../../vir_/vir.h"
#include <filesystem>

namespace emul::executable {
struct Program {
    machine::Program machine;
    vir::Program host;
};
Program read(const std::filesystem::path& path);
}
