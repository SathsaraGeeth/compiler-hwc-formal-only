/*
 * compiler/eir/include/MIR/MachineOperand.h
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
 * 1. Typed machine instruction operand.
 * 2. e.g., %value, @state, 4s<8>'h03, or 7.
 * 3. Attrs
 *    - kind; virtual register, state, literal, or immediate
 *    - text; canonical assembly spelling
 * 4. Methods
 *    - classify; constructs an operand from assembly text
 *    - is_value; true for a register or literal
 *    - immediate; parses an immediate integer
 *    - virtual_register; parses a VirtualRegister
 *    - operand_kind_name; returns the kind name
 */

#pragma once
#include "VirtualRegister.h"
#include <cstdint>
#include <string>
#include <string_view>

namespace emul::mir {
enum class MachineOperandKind {
    virtual_register,
    state,
    literal,
    immediate
};

struct MachineOperand {
    MachineOperandKind kind{};
    std::string text;
    static MachineOperand classify(std::string text);
    bool is_value() const noexcept;
    uint32_t immediate() const;
    VirtualRegister virtual_register() const;
};

std::string_view operand_kind_name(MachineOperandKind kind) noexcept;
}
