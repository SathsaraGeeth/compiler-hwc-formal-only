/*
 * compiler/eir/lib/MIR/MachineInstr.cpp
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

#include "../../include/MIR/MachineInstr.h"
#include <array>

namespace emul::mir {
namespace {
using Entry = std::pair<std::string_view, MachineOpcode>;

constexpr std::array opcodes{
    Entry{"and",         MachineOpcode::bit_and},
    Entry{"or",          MachineOpcode::bit_or},
    Entry{"not",         MachineOpcode::bit_not},
    Entry{"xor",         MachineOpcode::bit_xor},
    Entry{"mux",         MachineOpcode::mux},
    Entry{"redand",      MachineOpcode::redand},
    Entry{"redor",       MachineOpcode::redor},
    Entry{"redxor",      MachineOpcode::redxor},
    Entry{"add",         MachineOpcode::add},
    Entry{"sub",         MachineOpcode::sub},
    Entry{"mul",         MachineOpcode::mul},
    Entry{"shl",         MachineOpcode::shl},
    Entry{"lshr",        MachineOpcode::lshr},
    Entry{"ashr",        MachineOpcode::ashr},
    Entry{"eq",          MachineOpcode::eq},
    Entry{"ne",          MachineOpcode::ne},
    Entry{"slt",         MachineOpcode::slt},
    Entry{"sle",         MachineOpcode::sle},
    Entry{"ult",         MachineOpcode::ult},
    Entry{"ule",         MachineOpcode::ule},
    Entry{"slice",       MachineOpcode::slice},
    Entry{"concat",      MachineOpcode::concat},
    Entry{"zext",        MachineOpcode::zext},
    Entry{"sext",        MachineOpcode::sext},
    Entry{"trunc",       MachineOpcode::trunc},
    Entry{"state_read",  MachineOpcode::state_read},
    Entry{"state_write", MachineOpcode::state_write},
    Entry{"yield",       MachineOpcode::yield}
};
}

std::string_view opcode_name(MachineOpcode opcode) noexcept {
    for (const auto& [name, value] : opcodes) {
        if (value == opcode)
            return name;
    }
    return "invalid";
}

std::optional<MachineOpcode>
parse_opcode(std::string_view name) noexcept {
    for (const auto& [spelling, opcode] : opcodes) {
        if (spelling == name)
            return opcode;
    }
    return std::nullopt;
}
}
