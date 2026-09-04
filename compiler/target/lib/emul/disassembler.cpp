/*
 * compiler/target/lib/emul/disassembler.cpp
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
 * 1. Implements disassembly of emulator machine programs
 */

#include "target/emul/disassembler.h"
#include <array>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string_view>

namespace emul::machine {
namespace {
struct Decoded {
    uint8_t opcode, rd, rs1, rs2, rs3;
    uint32_t immediate;
};

Decoded decode(uint64_t stored_word) {
    const auto word = static_cast<uint32_t>(stored_word);
    return {static_cast<uint8_t>(word >> 26),
            static_cast<uint8_t>((word >> 21) & 31),
            static_cast<uint8_t>((word >> 16) & 31),
            static_cast<uint8_t>((word >> 11) & 31),
            static_cast<uint8_t>((word >> 6) & 31), word & 0xffffu};
}

std::string reg(uint8_t value) { return "x" + std::to_string(value); }

const Binding* binding(const Program& program, uint32_t address, BindingKind kind) {
    for (const auto& item : program.bindings)
        if (item.address == address && item.kind == kind) return &item;
    return nullptr;
}

std::string address(const Program& program, uint32_t value, BindingKind kind) {
    auto result = std::to_string(value);
    if (const auto* item = binding(program, value, kind)) result += " <" + item->name + ">";
    return result;
}

constexpr std::array<std::string_view, 57> names{
    "nop", "commit", "yield", "add", "sub", "mul", "shl", "lshr",
    "ashr", "and", "or", "not", "xor", "mux", "redand", "redor",
    "redxor", "eq", "ne", "slt", "sle", "ult", "ule", "concat",
    "zext", "sext", "reserved_1a", "reserved_1b", "reserved_1c",
    "reserved_1d", "reserved_1e", "reserved_1f", "slice", "addi", "subi",
    "muli", "shli", "lshri", "ashri", "andi", "ori", "xori",
    "redandi", "redori", "redxori", "eqi", "nei", "slti", "slei",
    "ulti", "ulei", "li", "state_read", "state_write", "import_read",
    "export_write", "literal"};

std::string instruction(const Program& program, const Decoded& value,
                        uint32_t extended, bool has_continuation) {
    std::ostringstream text;
    if (value.opcode >= names.size()) {
        text << ".word opcode=0x" << std::hex << unsigned(value.opcode);
        return text.str();
    }
    text << names[value.opcode];
    if (value.opcode <= 0x02) return text.str();
    if (value.opcode >= 0x03 && value.opcode <= 0x17) {
        text << ' ' << reg(value.rd) << ", " << reg(value.rs1);
        if (value.opcode != 0x0b && value.opcode != 0x0e &&
            value.opcode != 0x0f && value.opcode != 0x10)
            text << ", " << reg(value.rs2);
        if (value.opcode == 0x0d) text << ", " << reg(value.rs3);
        return text.str();
    }
    if (value.opcode == 0x18 || value.opcode == 0x19) {
        text << ' ' << reg(value.rd) << ", " << reg(value.rs1)
             << ", width " << ((value.immediate & 31) + 1);
        return text.str();
    }
    if (value.opcode == 0x20) {
        text << ' ' << reg(value.rd) << ", " << reg(value.rs1)
             << ", start " << (value.immediate & 63)
             << ", width " << (((value.immediate >> 6) & 31) + 1);
        return text.str();
    }
    if (value.opcode >= 0x21 && value.opcode <= 0x32) {
        text << ' ' << reg(value.rd) << ", " << reg(value.rs1)
             << ", " << value.immediate;
        return text.str();
    }
    if (value.opcode >= 0x33 && value.opcode <= 0x37) {
        const auto width = (value.rs1 & 31) + 1;
        text << ' ';
        if (value.opcode == 0x33)
            text << reg(value.rd) << ", "
                 << (has_continuation ? std::to_string(extended) : "<missing>");
        else if (value.opcode == 0x34)
            text << reg(value.rd) << ", " << address(program, extended, BindingKind::state);
        else if (value.opcode == 0x35)
            text << address(program, extended, BindingKind::state) << ", " << reg(value.rd);
        else if (value.opcode == 0x36)
            text << reg(value.rd) << ", " << address(program, extended, BindingKind::import);
        else
            text << address(program, extended, BindingKind::export_value) << ", " << reg(value.rd);
        text << " : width " << width;
        return text.str();
    }
    text << " 0x" << std::hex << extended;
    return text.str();
}
} // namespace

void disassemble(const Program& program, std::ostream& output) {
    for (size_t pc = 0; pc < program.words.size(); ++pc) {
        const auto word = static_cast<uint32_t>(program.words[pc]);
        const auto decoded = decode(word);
        const bool needs_continuation = decoded.opcode >= 0x33 && decoded.opcode <= 0x37;
        const bool has_continuation = needs_continuation && pc + 1 < program.words.size() &&
            (static_cast<uint32_t>(program.words[pc + 1]) >> 26) == 0x38;
        uint32_t extended = decoded.immediate;
        if (has_continuation)
            extended |= (static_cast<uint32_t>(program.words[pc + 1]) & 0xffffu) << 16;
        output << std::setfill('0') << std::setw(4) << std::hex << pc << ":  "
               << std::setw(8) << word << "  "
               << instruction(program, decoded, extended, has_continuation) << '\n';
        if (has_continuation) {
            ++pc;
            output << std::setfill('0') << std::setw(4) << std::hex << pc << ":  "
                   << std::setw(8) << static_cast<uint32_t>(program.words[pc])
                   << "  literal (continuation)\n";
        }
    }
    output << std::dec;
}
} // namespace emul::machine
