/*
 * compiler/eir/lib/CodeGen/Emit/ProgramDisassembler.cpp
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

#include "../../../include/CodeGen/Emit/ProgramDisassembler.h"
#include "target/emul/disassembler.h"
#include <ostream>

namespace emul::executable {
namespace {
const char* kind(machine::BindingKind value) {
    switch (value) {
        case machine::BindingKind::import: return "import";
        case machine::BindingKind::export_value: return "export";
        case machine::BindingKind::state: return "state";
    }
    return "unknown";
}
}

void disassemble(const Program& program, std::ostream& output) {
    output << "bindings:\n";
    for (const auto& binding : program.machine.bindings)
        output << "  " << kind(binding.kind) << ' ' << binding.address << " <" << binding.name
               << "> : " << (binding.four_state ? "4s<" : "2s<") << binding.width << ">\n";
    output << "\nisa:\n";
    machine::disassemble(program.machine, output);
    output << "\nhost vir:\n";
    vir::print(program.host, output);
}
}
