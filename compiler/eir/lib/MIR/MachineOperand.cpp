/*
 * compiler/eir/lib/MIR/MachineOperand.cpp
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

#include "../../include/MIR/MachineOperand.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace emul::mir {
MachineOperand MachineOperand::classify(std::string text) {
    MachineOperandKind kind = MachineOperandKind::literal;
    if (text.starts_with("%")) {
        kind = MachineOperandKind::virtual_register;
    } else if (text.starts_with("@")) {
        kind = MachineOperandKind::state;
    } else if (!text.empty() &&
               std::all_of(text.begin(), text.end(), [](unsigned char value) {
                   return std::isdigit(value);
               })) {
        kind = MachineOperandKind::immediate;
    }
    return {kind, std::move(text)};
}

bool MachineOperand::is_value() const noexcept {
    return kind == MachineOperandKind::virtual_register ||
           kind == MachineOperandKind::literal;
}

uint32_t MachineOperand::immediate() const {
    if (kind != MachineOperandKind::immediate)
        throw std::runtime_error("MIR operand is not an immediate");
    return static_cast<uint32_t>(std::stoul(text));
}

VirtualRegister MachineOperand::virtual_register() const {
    if (kind != MachineOperandKind::virtual_register)
        throw std::runtime_error("MIR operand is not a virtual register");
    return VirtualRegister::parse(text);
}

std::string_view operand_kind_name(MachineOperandKind kind) noexcept {
    switch (kind) {
    case MachineOperandKind::virtual_register:
        return "vreg";
    case MachineOperandKind::state:
        return "state";
    case MachineOperandKind::literal:
        return "literal";
    case MachineOperandKind::immediate:
        return "immediate";
    }
    return "invalid";
}
}
