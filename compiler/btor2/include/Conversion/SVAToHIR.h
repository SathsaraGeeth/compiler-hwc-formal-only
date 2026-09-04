/*
 * compiler/btor2/include/Conversion/SVAToHIR.h
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
 * Converts normalized SVA directives into higher level BTOR2 operations
 */

#pragma once
#include "HIR/Module.h"

namespace emul::btor2 {
hir::Opcode classify_ltl(const frontend::sva::Property& property);
std::vector<hir::Operation::Component> decompose_ltl(
    const frontend::sva::Property& property);
hir::Operation convert_sva_to_hir(
    const frontend::sva::Directive& directive);
hir::Module convert_sva_to_hir(
    const std::vector<frontend::sva::Directive>& directives);
}
