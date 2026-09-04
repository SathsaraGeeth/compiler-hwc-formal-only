/*
 * compiler/eir/include/CodeGen/Emit/ProgramWriter.h
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
void write(const std::filesystem::path& path, const machine::Program& machine,
           const vir::Program& vir);
}
