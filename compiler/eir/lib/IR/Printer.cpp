/*
 * compiler/eir/lib/IR/Printer.cpp
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

#include "../../include/IR/Printer.h"
#include <array>
#include <ostream>
#include <stdexcept>
#include <string_view>

namespace emul::eir {
namespace {
bool known(std::string_view opcode) {
    static constexpr std::array names{"and", "or", "not", "xor", "mux",
        "redand", "redor", "redxor", "add", "sub", "mul", "shl", "lshr",
        "ashr", "eq", "ne", "slt", "sle", "ult", "ule", "slice", "concat",
        "zext", "sext", "trunc", "state_read", "state_write", "import_read",
        "export_write", "instance", "yield"};
    for (auto name : names)
        if (opcode == name)
            return true;
    return false;
}
}
void print(const Program& program, std::ostream& output) {
    output << "eir.version 1\n\n";
    for (const auto& module : program.modules) {
        output << "module @" << module.name << "(";
        for (size_t index = 0; index < module.inputs.size(); ++index) {
            if (index) output << ", ";
            output << module.inputs[index].name << " : " << module.inputs[index].type;
        }
        output << ")";
        if (!module.results.empty()) {
            output << " -> (";
            for (size_t index = 0; index < module.results.size(); ++index) {
                if (index) output << ", ";
                output << module.results[index].name << " : " << module.results[index].type;
            }
            output << ")";
        }
        output << " {\n";
        for (const auto& state : module.states)
            output << "  state " << state.name << " : " << state.type << "\n";
        for (const auto& operation : module.operations) {
            if (!known(operation.opcode))
                throw std::runtime_error("operation is not defined by spec/ir.yaml: " +
                                         operation.opcode);
            output << "  ";
            if (!operation.result.empty()) {
                output << operation.result;
                if (!operation.result_type.empty()) output << " : " << operation.result_type;
                output << " = ";
            }
            output << operation.opcode
                   << (operation.operands.empty() ? "" : " ")
                   << operation.operands << "\n";
        }
        output << "}\nendmodule\n\n";
    }
}
}
