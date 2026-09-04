/*
 * compiler/target/lib/emul/emul_target.cpp
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
 * 1. Implements the FPGA emulator target description.
 */

#include "target/emul/emul_target.h"
#include <stdexcept>
#include <unordered_map>

namespace emul::target::emulator {
/* Q: How many registers does the emulator ISA expose? */
/* A: Thirty two five bit register numbers, x0 through x31 */
uint8_t EmulatorTarget::register_count() const { return 32; }

/* Q: Which emulator register is available first to the allocator? */
/* A: x1; x0 is reserved by convention. */
uint8_t EmulatorTarget::first_allocatable_register() const { return 1; }

std::optional<uint8_t> EmulatorTarget::opcode(std::string_view operation) const {
    /*
     * Q: Which opcode implements an EIR operation on the emulator?
     * A: Return the fixed opcode table entry, or no value if unsupported
     */
    static const std::unordered_map<std::string_view, uint8_t> opcodes{
        {"nop",0x00},{"commit",0x01},{"yield",0x02},
        {"add",0x03},{"sub",0x04},{"mul",0x05},{"shl",0x06},
        {"lshr",0x07},{"ashr",0x08},{"and",0x09},{"or",0x0a},
        {"not",0x0b},{"xor",0x0c},{"mux",0x0d},{"redand",0x0e},
        {"redor",0x0f},{"redxor",0x10},{"eq",0x11},{"ne",0x12},
        {"slt",0x13},{"sle",0x14},{"ult",0x15},{"ule",0x16},
        {"concat",0x17},{"zext",0x18},{"sext",0x19},
        {"slice",0x20},{"addi",0x21},{"subi",0x22},{"muli",0x23},
        {"shli",0x24},{"lshri",0x25},{"ashri",0x26},{"andi",0x27},
        {"ori",0x28},{"xori",0x29},{"redandi",0x2a},
        {"redori",0x2b},{"redxori",0x2c},{"eqi",0x2d},
        {"nei",0x2e},{"slti",0x2f},{"slei",0x30},{"ulti",0x31},
        {"ulei",0x32},{"li",0x33},{"state_read",0x34},
        {"state_write",0x35},{"import_read",0x36},{"export_write",0x37},
        {"literal",0x38}
    };
    auto found = opcodes.find(operation);
    return found == opcodes.end() ? std::nullopt : std::optional<uint8_t>(found->second);
}

uint32_t EmulatorTarget::encode(uint8_t opcode, uint8_t destination, uint8_t source1,
                                uint8_t source2, uint8_t source3, ValueType type,
                                uint32_t immediate) const {
    /*
     * Q: How are emulator instruction fields packed?
     * A: Pack them into the frozen 32-bit ZERO, R4, R2I, or RI format.
     */
    if (type.width > 32)
        throw std::runtime_error("emulator ISA value width exceeds 32 bits");
    const uint32_t op = uint32_t(opcode) << 26;
    if (opcode <= 0x02) return op;
    if (opcode >= 0x03 && opcode <= 0x17)
        return op | uint32_t(destination) << 21 | uint32_t(source1) << 16 |
               uint32_t(source2) << 11 | uint32_t(source3) << 6;
    if (opcode == 0x18 || opcode == 0x19 ||
        (opcode >= 0x20 && opcode <= 0x32)) {
        if (immediate > 0xffffu)
            throw std::runtime_error("emulator R2I immediate exceeds 16 bits");
        return op | uint32_t(destination) << 21 | uint32_t(source1) << 16 |
               immediate;
    }
    if (opcode == 0x38) {
        if (immediate > 0x3ffffffu)
            throw std::runtime_error("emulator I immediate exceeds 26 bits");
        return op | immediate;
    }
    if (opcode >= 0x33 && opcode <= 0x37) {
        if (!type.width)
            throw std::runtime_error("emulator RI instruction requires a value type");
        if (immediate > 0xffffu)
            throw std::runtime_error("emulator RI low immediate exceeds 16 bits");
        immediate |= (type.width - 1) << 16;
        return op | uint32_t(destination ? destination : source1) << 21 |
               immediate;
    }
    throw std::runtime_error("reserved emulator ISA opcode");
}

const EmulatorTarget& get_target() {
    static const EmulatorTarget target;
    return target;
}
} // namespace emul::target::emulator
