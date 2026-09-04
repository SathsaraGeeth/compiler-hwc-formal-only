/*
 * compiler/eir/include/MIR/MachineInstr.h
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
 * 1. Machine instruction.
 * 2. e.g., %3 : 4s<8> = add %a, %b
 * 3. MachineOpcode is the concrete opcode set in spec/mir.yaml.
 * 4. Attrs
 *    - result; destination register, e.g. "%3"
 *    - result_type; e.g. "4s<8>"
 *    - opcode; e.g. MachineOpcode::add
 *    - operands; typed machine operands
 * 5. Functions
 *    - opcode_name; converts an opcode into assembly text
 *    - parse_opcode; converts assembly text into an opcode
 */

#pragma once
#include "MachineOperand.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace emul::mir {
enum class MachineOpcode {
    bit_and,
    bit_or,
    bit_not,
    bit_xor,
    mux,
    redand,
    redor,
    redxor,
    add,
    sub,
    mul,
    shl,
    lshr,
    ashr,
    eq,
    ne,
    slt,
    sle,
    ult,
    ule,
    slice,
    concat,
    zext,
    sext,
    trunc,
    state_read,
    state_write,
    yield
};

std::string_view opcode_name(MachineOpcode opcode) noexcept;
std::optional<MachineOpcode> parse_opcode(std::string_view name) noexcept;

struct MachineInstr {
    std::string result;
    std::string result_type;
    MachineOpcode opcode{};
    std::vector<MachineOperand> operands;
};
}
