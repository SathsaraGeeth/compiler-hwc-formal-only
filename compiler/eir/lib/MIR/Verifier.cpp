/*
 * compiler/eir/lib/MIR/Verifier.cpp
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

#include "../../include/MIR/Verifier.h"
#include "../../include/IR/Type.h"
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace emul::mir {
namespace {
using Kind = MachineOperandKind;
bool value(Kind kind) {
    return kind == Kind::virtual_register || kind == Kind::literal;
}
void count(const MachineInstr& instruction, size_t expected) {
    if (instruction.operands.size() != expected)
        throw std::runtime_error("MIR " + std::string(opcode_name(instruction.opcode)) +
                                 " expects " + std::to_string(expected) + " operands");
}
void kind(const MachineInstr& instruction, size_t index, Kind expected) {
    if (instruction.operands[index].kind != expected)
        throw std::runtime_error("MIR " + std::string(opcode_name(instruction.opcode)) +
            " operand " + std::to_string(index) + " must be " +
            std::string(operand_kind_name(expected)));
}
void value_kind(const MachineInstr& instruction, size_t index) {
    if (!value(instruction.operands[index].kind))
        throw std::runtime_error("MIR " + std::string(opcode_name(instruction.opcode)) +
            " operand " + std::to_string(index) + " must be a vreg or literal");
}
void signature(const MachineInstr& instruction) {
    switch (instruction.opcode) {
    case MachineOpcode::bit_not:
    case MachineOpcode::redand:
    case MachineOpcode::redor:
    case MachineOpcode::redxor:
    case MachineOpcode::zext:
    case MachineOpcode::sext:
    case MachineOpcode::trunc:
        count(instruction, 1);
        value_kind(instruction, 0);
        return;
    case MachineOpcode::bit_and:
    case MachineOpcode::bit_or:
    case MachineOpcode::bit_xor:
    case MachineOpcode::add:
    case MachineOpcode::sub:
    case MachineOpcode::mul:
    case MachineOpcode::shl:
    case MachineOpcode::lshr:
    case MachineOpcode::ashr:
    case MachineOpcode::eq:
    case MachineOpcode::ne:
    case MachineOpcode::slt:
    case MachineOpcode::sle:
    case MachineOpcode::ult:
    case MachineOpcode::ule:
    case MachineOpcode::concat:
        count(instruction, 2);
        value_kind(instruction, 0);
        value_kind(instruction, 1);
        return;
    case MachineOpcode::mux:
        count(instruction, 3);
        value_kind(instruction, 0);
        value_kind(instruction, 1);
        value_kind(instruction, 2);
        return;
    case MachineOpcode::slice:
        count(instruction, 3);
        value_kind(instruction, 0);
        kind(instruction, 1, Kind::immediate);
        kind(instruction, 2, Kind::immediate);
        return;
    case MachineOpcode::state_read:
        count(instruction, 1);
        kind(instruction, 0, Kind::state);
        return;
    case MachineOpcode::state_write:
        count(instruction, 2);
        kind(instruction, 0, Kind::state);
        value_kind(instruction, 1);
        return;
    case MachineOpcode::yield:
        for (size_t index = 0; index < instruction.operands.size(); ++index)
            value_kind(instruction, index);
        return;
    }
}
bool produces_value(MachineOpcode opcode) {
    return opcode != MachineOpcode::state_write && opcode != MachineOpcode::yield;
}
}

void verify(const MachineModule& module) {
    if (module.functions.empty())
        throw std::runtime_error("MIR module has no function");
    std::unordered_set<std::string> functions;
    for (const auto& function : module.functions) {
        if (function.name.empty() || !functions.insert(function.name).second)
            throw std::runtime_error(
                "invalid or duplicate MIR function " + function.name);
        if (function.blocks.size() != 1)
            throw std::runtime_error(
                "MIR function must have exactly one block @" + function.name);
        std::unordered_set<std::string> states;
        for (const auto& state : function.states) {
            eir::Type::parse(state.type);
            if (!states.insert(state.name).second)
                throw std::runtime_error("duplicate MIR state " + state.name);
        }
        std::unordered_map<std::string, std::string> values;
        for (const auto& input : function.inputs) {
            eir::Type::parse(input.type);
            VirtualRegister::parse(input.name);
            if (!values.emplace(input.name, input.type).second)
                throw std::runtime_error("duplicate MIR input " + input.name);
        }
        for (const auto& result : function.results)
            eir::Type::parse(result.type);
        bool yielded = false;
        for (const auto& block : function.blocks) {
            if (block.name.empty())
                throw std::runtime_error("empty MIR block name");
            for (size_t index = 0; index < block.instructions.size(); ++index) {
                const auto& instruction = block.instructions[index];
                signature(instruction);
                for (const auto& operand : instruction.operands) {
                    if (operand.kind == Kind::virtual_register) {
                        operand.virtual_register();
                        if (!values.contains(operand.text))
                            throw std::runtime_error(
                                "undefined MIR vreg " + operand.text);
                    }
                    if (operand.kind == Kind::state &&
                        !states.contains(operand.text))
                        throw std::runtime_error(
                            "undefined MIR state " + operand.text);
                    if (operand.kind == Kind::immediate)
                        operand.immediate();
                }
                if (produces_value(instruction.opcode)) {
                    if (instruction.result.empty())
                        throw std::runtime_error("MIR value instruction has no result");
                    VirtualRegister::parse(instruction.result);
                    eir::Type::parse(instruction.result_type);
                    auto [value, inserted] = values.emplace(
                        instruction.result, instruction.result_type);
                    if (!inserted && value->second != instruction.result_type)
                        throw std::runtime_error(
                            "MIR vreg reassigned with a different type " +
                            instruction.result);
                } else if (!instruction.result.empty() ||
                           !instruction.result_type.empty()) {
                    throw std::runtime_error(
                        "MIR effect instruction cannot define a result");
                }
                if (instruction.opcode == MachineOpcode::yield) {
                    if (yielded || index + 1 != block.instructions.size())
                        throw std::runtime_error(
                            "MIR yield must be the unique final instruction");
                    if (instruction.operands.size() != function.results.size())
                        throw std::runtime_error("MIR yield result count mismatch");
                    yielded = true;
                }
            }
        }
        if (!yielded)
            throw std::runtime_error(
                "MIR function has no yield @" + function.name);
    }
}
}
