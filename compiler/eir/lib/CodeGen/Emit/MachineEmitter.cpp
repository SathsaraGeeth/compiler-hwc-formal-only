/*
 * compiler/eir/lib/CodeGen/Emit/MachineEmitter.cpp
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

#include "../../../include/CodeGen/Emit/MachineEmitter.h"
#include "../../../include/Lowering/LowerToMIR.h"
#include "../../../include/MIR/Verifier.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace emul::machine {
namespace {
using Type = target::ValueType;
Type type(std::string_view text) {
    auto begin = text.find('<'), end = text.find('>');
    return {static_cast<uint32_t>(std::stoul(std::string(text.substr(begin + 1, end - begin - 1)))),
            text.starts_with("4s")};
}
struct Literal { Type type; uint64_t bits = 0; uint32_t kind = 0; };
Literal literal(std::string text) {
    auto quote = text.find('\'');
    auto result = Literal{type(text.substr(0, quote))};
    auto value = text.substr(quote + 1);
    if (value == "x") result.kind = 1;
    else if (value == "z") result.kind = 2;
    else if (value.starts_with("0x")) result.bits = std::stoull(value.substr(2), nullptr, 16);
    else throw std::runtime_error("invalid EIR literal " + text);
    return result;
}

class Encoder {
public:
    Encoder(const mir::MachineModule& source, const target::Target& target)
        : module(source.root()), target_(target),
          used(target.register_count()) {
        pinned.resize(target.register_count());
        for (const auto& state : module.states) {
            types[state.name] = type(state.type);
            address(state.name, BindingKind::state, state.type);
        }
        for (const auto& input : module.inputs) types[input.name] = type(input.type);
        for (const auto& operation : module.blocks.front().instructions) {
            if (operation.opcode == mir::MachineOpcode::state_read)
                ++remaining_state_reads;
            for (std::size_t index = 0; index < operation.operands.size(); ++index) {
                const auto& operand = operation.operands[index];
                if (operation.opcode == mir::MachineOpcode::yield &&
                    operand.kind == mir::MachineOperandKind::virtual_register) {
                    eager_exports[operand.text].push_back(index);
                    continue;
                }
                if (operand.kind == mir::MachineOperandKind::virtual_register)
                    ++references[operand.text];
            }
            if (!operation.result.empty() && !operation.result_type.empty())
                types[operation.result] = type(operation.result_type);
        }
    }
    Program run() {
        for (const auto& input : module.inputs) {
            auto addr = address(input.name.substr(1), BindingKind::import, input.type);
            input_addresses[input.name] = addr;
        }
        for (const auto& operation : module.blocks.front().instructions) lower(operation);
        return std::move(program);
    }
private:
    mir::MachineFunction module;
    Program program;
    std::unordered_map<std::string, Type> types;
    std::unordered_map<std::string, uint32_t> references;
    std::unordered_map<std::string, uint8_t> registers;
    std::unordered_map<std::string, std::vector<std::size_t>> eager_exports;
    std::unordered_map<std::string, uint32_t> input_addresses;
    std::unordered_map<std::string, uint32_t> state_addresses;
    std::unordered_map<std::string, uint32_t> spills;
    const target::Target& target_;
    std::vector<bool> used;
    std::vector<bool> pinned;
    uint32_t pending_state_writes = 0;
    uint32_t remaining_state_reads = 0;
    uint64_t next_spill_id = 0;
    std::string current_operation;

    uint8_t opcode(std::string_view operation) const {
        auto result = target_.opcode(operation);
        if (!result)
            throw std::runtime_error(
                "target does not implement " + std::string(operation));
        return *result;
    }
    void emit(std::string_view operation, uint8_t rd = 0, uint8_t rs1 = 0,
              uint8_t rs2 = 0, uint8_t rs3 = 0, Type result = {},
              uint32_t immediate = 0) {
        program.words.push_back(target_.encode(opcode(operation), rd, rs1, rs2, rs3,
                                               result, immediate));
    }
    void emit_word(uint64_t word) { program.words.push_back(word); }
    void emit_ri(std::string_view operation, uint8_t rd, uint8_t rs1,
                 Type value_type, uint32_t immediate) {
        emit(operation, rd, rs1, 0, 0, value_type, immediate & 0xffffu);
        emit("literal", 0, 0, 0, 0, {}, immediate >> 16);
    }
    uint32_t address(const std::string& name, BindingKind kind, const std::string& type_name) {
        for (const auto& binding : program.bindings)
            if (binding.name == name && binding.kind == kind) return binding.address;
        uint32_t result = static_cast<uint32_t>(program.bindings.size());
        auto value_type = type(type_name);
        program.bindings.push_back({name, kind, result, value_type.width, value_type.four_state});
        return result;
    }
    uint8_t free_register() {
        for (uint8_t reg = target_.first_allocatable_register();
             reg < target_.register_count(); ++reg)
            if (!used[reg]) { used[reg] = true; return reg; }
        {
            for (auto item = registers.begin(); item != registers.end(); ++item) {
                auto reg = item->second;
                if (pinned[reg]) continue;
                auto spill_name = "@.__spill." + std::to_string(next_spill_id++);
                auto addr = address(spill_name, BindingKind::state,
                                    type_name(item->first));
                emit_ri("export_write", 0, reg, types.at(item->first), addr);
                spills[item->first] = addr;
                registers.erase(item);
                return reg;
            }
        }
        std::string live;
        for (const auto& [value, reg] : registers) {
            if (!live.empty()) live += ", ";
            live += value + "=x" + std::to_string(reg);
        }
        throw std::runtime_error("ISA register allocation failed while lowering " +
                                 current_operation + "; live values: " + live);
    }
    uint8_t allocate(const std::string& value) {
        auto allocated = registers.find(value);
        if (allocated != registers.end())
            return allocated->second;
        auto reg = free_register();
        registers[value] = reg;
        return reg;
    }
    uint8_t source(const std::string& operand, std::vector<uint8_t>& temporaries) {
        if (operand.starts_with("%")) {
            if (auto allocated = registers.find(operand); allocated != registers.end()) {
                pinned[allocated->second] = true;
                return allocated->second;
            }
            if (auto spilled = spills.find(operand); spilled != spills.end()) {
                auto reg = allocate(operand);
                emit_ri("import_read", reg, 0, types.at(operand), spilled->second);
                pinned[reg] = true;
                return reg;
            }
            if (auto state = state_addresses.find(operand);
                state != state_addresses.end()) {
                auto reg = allocate(operand);
                emit_ri("state_read", reg, 0, types.at(operand), state->second);
                pinned[reg] = true;
                return reg;
            }
            auto input = input_addresses.find(operand);
            if (input == input_addresses.end()) return registers.at(operand);
            auto reg = allocate(operand);
            emit_ri("import_read", reg, 0, types.at(operand), input->second);
            pinned[reg] = true;
            return reg;
        }
        auto value = literal(operand);
        if (value.kind) {
            auto reg = free_register();
            pinned[reg] = true;
            temporaries.push_back(reg);
            auto name = std::string("@.__literal.") +
                (value.kind == 1 ? "x." : "z.") + std::to_string(value.type.width);
            auto addr = address(name, BindingKind::state,
                                std::string(value.type.four_state ? "4s<" : "2s<") +
                                std::to_string(value.type.width) + ">");
            emit_ri("import_read", reg, 0, value.type, addr);
            return reg;
        }
        if (value.bits > 0xffffffffu)
            throw std::runtime_error(
                "emulator ISA literal exceeds 32 bits");
        auto reg = free_register();
        pinned[reg] = true;
        temporaries.push_back(reg);
        emit_ri("li", reg, 0, value.type, static_cast<uint32_t>(value.bits));
        return reg;
    }
    void release_sources(const std::vector<std::string>& operands,
                         const std::vector<uint8_t>& temporaries) {
        for (auto reg : temporaries) used[reg] = false;
        for (const auto& operand : operands) {
            if (!operand.starts_with("%")) continue;
            if (--references[operand] == 0) {
                if (auto allocated = registers.find(operand);
                    allocated != registers.end()) {
                    used[allocated->second] = false;
                    registers.erase(allocated);
                }
                spills.erase(operand);
            }
        }
    }
    void lower(const mir::MachineInstr& operation) {
        current_operation = std::string(mir::opcode_name(operation.opcode));
        std::fill(pinned.begin(), pinned.end(), false);
        std::vector<std::string> operands;
        for (const auto& operand : operation.operands) operands.push_back(operand.text);
        std::vector<uint8_t> temporary_registers;
        std::vector<uint8_t> source_registers;
        auto register_operands = operands;
        if (operation.opcode == mir::MachineOpcode::state_read ||
            operation.opcode == mir::MachineOpcode::state_write ||
            operation.opcode == mir::MachineOpcode::slice ||
            operation.opcode == mir::MachineOpcode::yield) {
            register_operands.clear();
            if (operation.opcode == mir::MachineOpcode::state_write)
                register_operands.push_back(operands[1]);
            else if (operation.opcode == mir::MachineOpcode::slice)
                register_operands.push_back(operands[0]);
        }
        for (const auto& operand : register_operands)
            source_registers.push_back(source(operand, temporary_registers));
        if (operation.opcode == mir::MachineOpcode::state_write) {
            auto addr = address(operands[0], BindingKind::state, type_name(operands[1]));
            emit_ri("state_write", 0, source_registers[0],
                    operand_type(operands[1]), addr);
            ++pending_state_writes;
        } else if (operation.opcode == mir::MachineOpcode::state_read) {
            auto addr = address(operands[0], BindingKind::state, operation.result_type);
            state_addresses[operation.result] = addr;
            --remaining_state_reads;
        } else if (operation.opcode == mir::MachineOpcode::yield) {
            if (pending_state_writes) {
                emit("commit");
                pending_state_writes = 0;
            }
            for (size_t index = 0; index < operands.size(); ++index) {
                if (operands[index].starts_with("%") &&
                    !input_addresses.contains(operands[index]) &&
                    !state_addresses.contains(operands[index])) continue;
                std::vector<uint8_t> literal_regs;
                auto reg = source(operands[index], literal_regs);
                auto addr = address(module.results[index].name, BindingKind::export_value,
                                    module.results[index].type);
                emit_ri("export_write", 0, reg, type(module.results[index].type), addr);
                if (input_addresses.contains(operands[index]) ||
                    state_addresses.contains(operands[index])) {
                    used[reg] = false;
                    registers.erase(operands[index]);
                } else {
                    release_sources({operands[index]}, literal_regs);
                }
            }
            emit("commit");
            emit("yield");
            return;
        } else if (operation.opcode == mir::MachineOpcode::mux) {
            auto result = allocate(operation.result);
            pinned[result] = true;
            auto result_type = types.at(operation.result);
            auto mask = free_register();
            emit("sext", mask, source_registers[0], 0, 0,
                 result_type, result_type.width - 1);
            auto inverted = free_register();
            emit("not", inverted, mask, 0, 0, result_type);
            auto when_false = free_register();
            emit("and", when_false, source_registers[1], inverted, 0,
                 result_type);
            used[inverted] = false;
            auto when_true = free_register();
            emit("and", when_true, source_registers[2], mask, 0,
                 result_type);
            emit("or", result, when_false, when_true, 0, result_type);
            used[mask] = false;
            used[when_false] = false;
            used[when_true] = false;
        } else {
            auto reg = allocate(operation.result);
            uint32_t immediate = 0;
            auto operation_name = mir::opcode_name(operation.opcode);
            if (operation.opcode == mir::MachineOpcode::slice) {
                const auto start = operation.operands[1].immediate();
                const auto result_width = types.at(operation.result).width;
                if (start > 31 || !result_width || result_width > 32)
                    throw std::runtime_error("emulator ISA slice is out of range");
                immediate = start | ((result_width - 1) << 6);
            } else if (operation.opcode == mir::MachineOpcode::zext ||
                       operation.opcode == mir::MachineOpcode::sext) {
                immediate = types.at(operation.result).width - 1;
            } else if (operation.opcode == mir::MachineOpcode::trunc) {
                operation_name = "slice";
                immediate = (types.at(operation.result).width - 1) << 6;
            }
            emit(operation_name, reg,
                source_registers.size() > 0 ? source_registers[0] : 0,
                source_registers.size() > 1 ? source_registers[1] : 0,
                source_registers.size() > 2 ? source_registers[2] : 0,
                types.at(operation.result), immediate);
        }
        if (!operation.result.empty()) {
            auto exports = eager_exports.find(operation.result);
            if (exports != eager_exports.end() && registers.contains(operation.result)) {
                auto reg = registers.at(operation.result);
                for (auto index : exports->second) {
                    auto addr = address(module.results[index].name,
                                        BindingKind::export_value,
                                        module.results[index].type);
                    emit_ri("export_write", 0, reg,
                            type(module.results[index].type), addr);
                }
            }
        }
        release_sources(register_operands, temporary_registers);
        std::fill(pinned.begin(), pinned.end(), false);
        if (!operation.result.empty() && references[operation.result] == 0) {
            if (auto allocated = registers.find(operation.result);
                allocated != registers.end()) {
                used[allocated->second] = false;
                registers.erase(allocated);
            }
            spills.erase(operation.result);
        }
    }
    Type operand_type(const std::string& value) {
        auto found = types.find(value);
        return found == types.end() ? literal(value).type : found->second;
    }
    std::string type_name(const std::string& value) {
        const auto value_type = operand_type(value);
        return std::string(value_type.four_state ? "4s<" : "2s<") +
               std::to_string(value_type.width) + ">";
    }
};
}

Program encode(const eir::Program& program, const target::Target& target) {
    return encode(eir::lowering::lower_to_mir(program), target);
}
Program encode(const mir::MachineModule& module, const target::Target& target) {
    mir::verify(module);
    return Encoder(module, target).run();
}
}
