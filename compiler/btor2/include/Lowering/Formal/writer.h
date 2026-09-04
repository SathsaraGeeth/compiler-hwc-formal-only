/*
 * compiler/btor2/include/Lowering/Formal/writer.h
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
 * Builds and writes a named formal transition system
 */

#pragma once
#include "frontend/elaborated_design.h"
#include "IR/Module.h"
#include <filesystem>
#include <iosfwd>
#include <string_view>

namespace emul::formal {
enum class PropertyTarget { all, safety, liveness };

btor2::Module build_transition_system(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    bool optimize = true);

void write_transition_system(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    const std::filesystem::path& output);

void write_transition_system(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    std::ostream& output,
    PropertyTarget target = PropertyTarget::all,
    bool optimize = true);
}
