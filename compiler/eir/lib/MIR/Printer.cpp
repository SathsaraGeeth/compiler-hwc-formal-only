/*
 * compiler/eir/lib/MIR/Printer.cpp
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

#include "../../include/MIR/Printer.h"
#include <ostream>

namespace emul::mir {
void print(const MachineModule& module, std::ostream& output) {
    output << "mir.version 1\n\n";
    for (const auto& function : module.functions) {
        output << "machine @" << function.name << "(";
        for (size_t index = 0; index < function.inputs.size(); ++index) {
            if (index) output << ", ";
            output << function.inputs[index].name << " : "
                   << function.inputs[index].type;
        }
        output << ")";
        if (!function.results.empty()) {
            output << " -> (";
            for (size_t index = 0; index < function.results.size(); ++index) {
                if (index) output << ", ";
                output << function.results[index].name << " : "
                       << function.results[index].type;
            }
            output << ")";
        }
        output << " {\n";
        for (const auto& state : function.states)
            output << "  state " << state.name << " : " << state.type << "\n";
        for (const auto& block : function.blocks) {
            output << "^" << block.name << ":\n";
            for (const auto& instruction : block.instructions) {
                output << "  ";
                if (!instruction.result.empty())
                    output << instruction.result << " : "
                           << instruction.result_type << " = ";
                output << opcode_name(instruction.opcode);
                for (size_t index = 0; index < instruction.operands.size(); ++index)
                    output << (index ? ", " : " ") << instruction.operands[index].text;
                output << "\n";
            }
        }
        output << "}\nendmachine\n\n";
    }
}
}
