/*
 * compiler/btor2/lib/Lowering/TwoStateEIRLowering.cpp
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

#include "Lowering/TwoStateEIRLowering.h"
#include "eir/include/Lowering/Flatten.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace emul::btor2 {
namespace {
std::vector<std::string> split(std::string_view text) {
    std::vector<std::string> result;
    if (text.empty())
        return result;
    size_t start = 0;
    for (size_t index = 0; index <= text.size(); ++index) {
        if (index != text.size() && text[index] != ',')
            continue;
        auto item = text.substr(start, index - start);
        auto first = item.find_first_not_of(' ');
        auto last = item.find_last_not_of(' ');
        result.emplace_back(first == item.npos ? std::string_view{} :
                            item.substr(first, last - first + 1));
        start = index + 1;
    }
    return result;
}

std::string bare(std::string name) {
    if (!name.empty() && (name.front() == '%' || name.front() == '@'))
        name.erase(name.begin());
    return name;
}

uint64_t integer(std::string_view text) {
    auto quote = text.find('\'');
    auto payload = quote == text.npos ? text : text.substr(quote + 1);
    auto base = 10;
    if (payload.starts_with("0x") || payload.starts_with("0X")) {
        base = 16;
        payload.remove_prefix(2);
    } else if (payload.starts_with("0o") || payload.starts_with("0O")) {
        base = 8;
        payload.remove_prefix(2);
    } else if (payload.starts_with("0b") || payload.starts_with("0B")) {
        base = 2;
        payload.remove_prefix(2);
    } else if (payload.starts_with("0d") || payload.starts_with("0D")) {
        payload.remove_prefix(2);
    }
    auto digits = payload;
    std::string compact(digits);
    compact.erase(std::remove(compact.begin(), compact.end(), '_'),
                  compact.end());
    return std::stoull(compact, nullptr, base);
}

uint32_t literal_width(std::string_view text) {
    auto type_end = text.find('>');
    if (type_end == text.npos)
        throw std::runtime_error("untyped EIR formal literal: " +
                                 std::string(text));
    return eir::Type::parse(text.substr(0, type_end + 1)).width();
}

std::string binary_bits(uint64_t value, uint32_t width) {
    std::string result(width, '0');
    for (uint32_t index = 0; index < width && index < 64; ++index)
        if ((value >> index) & 1)
            result[width - index - 1] = '1';
    return result;
}

class Lowerer {
public:
    explicit Lowerer(const eir::Module& module) : module_(module) {}

    TwoStateTransitionSystem run() {
        lower_inputs();
        lower_states();
        for (const auto& operation : module_.operations)
            lower(operation);
        finish_states();
        return std::move(system_);
    }

private:
    const eir::Module& module_;
    TwoStateTransitionSystem system_;
    std::unordered_map<std::string, Btor2Value> values_;
    std::unordered_map<std::string, Btor2Value> states_;
    std::unordered_map<std::string, Btor2Value> next_states_;
    size_t nondeterministic_ = 0;

    void lower_inputs() {
        for (const auto& input : module_.inputs) {
            auto value = system_.builder.input(
                input.value_type().width(), bare(input.name));
            values_[input.name] = value;
            system_.signals[bare(input.name)] = value;
        }
    }

    void lower_states() {
        auto zero = system_.builder.constant(1, "0");
        auto initialized = system_.builder.state(1, "formal_initialized");
        system_.builder.init(initialized, zero);
        system_.builder.next(
            initialized, system_.builder.constant(1, "1"));
        system_.signals["formal_initialized"] = initialized;
        for (const auto& state : module_.states) {
            auto state_name = bare(state.name);
            auto state_type = state.value_type();
            Btor2Value initial_value;
            if (state_type.two_state())
                initial_value = system_.builder.constant(
                    state_type.width(),
                    std::string(state_type.width(), '0'));
            auto value = system_.builder.state(
                state_type.width(), state_name);
            states_[state.name] = value;
            system_.signals[state_name] = value;
            
            
            
            
            if (state_type.two_state())
                system_.builder.init(value, initial_value);
            auto leaf = state_name.substr(state_name.rfind('.') + 1);
            if (leaf.ends_with("_prev")) {
                leaf.resize(leaf.size() - 5);
                auto input = system_.signals.find(leaf);
                if (input != system_.signals.end() &&
                    input->second.width == value.width) {
                    auto equal = system_.builder.binary(
                        "eq", value, input->second, 1);
                    system_.builder.constraint(
                        system_.builder.binary(
                            "or", initialized, equal, 1),
                        state_name + ".initial_edge");
                }
            }
        }
    }

    Btor2Value operand(const std::string& text) {
        if (auto found = values_.find(text); found != values_.end())
            return found->second;
        if (auto found = states_.find(text); found != states_.end())
            return found->second;
        auto width = literal_width(text);
        auto quote = text.find('\'');
        auto digits = std::string_view(text).substr(quote + 1);
        if (digits.size() >= 2 && digits.front() == '0' &&
            std::string_view("xXoObBdD").find(digits[1]) !=
                std::string_view::npos)
            digits.remove_prefix(2);
        if (digits.find_first_of("xXzZ?") != std::string::npos)
            return system_.builder.input(
                width, "nondet_" + std::to_string(nondeterministic_++));
        return system_.builder.constant(
            width, binary_bits(integer(text), width));
    }

    void bind(const eir::Operation& operation, Btor2Value value) {
        values_[operation.result] = value;
        system_.signals[bare(operation.result)] = value;
    }

    Btor2Value resize(Btor2Value value, uint32_t width) {
        if (value.width == width)
            return value;
        if (value.width < width)
            return system_.builder.extend("uext", value, width);
        return system_.builder.slice(value, 0, width);
    }

    Btor2Value boolean(Btor2Value value) {
        if (value.width == 1)
            return value;
        auto zero = system_.builder.constant(
            value.width, std::string(value.width, '0'));
        return system_.builder.binary("neq", value, zero, 1);
    }

    void lower(const eir::Operation& operation) {
        auto arguments = split(operation.operands);
        auto opcode = operation.opcode;
        if (opcode == "state_read") {
            bind(operation, states_.at(arguments.at(0)));
            return;
        }
        if (opcode == "state_write") {
            next_states_[arguments.at(0)] = operand(arguments.at(1));
            return;
        }
        if (opcode == "yield") {
            for (size_t index = 0; index < arguments.size(); ++index) {
                auto value = operand(arguments[index]);
                auto name = module_.results.at(index).name;
                for (const auto& [state_name, state] : states_)
                    if (state_name == "@" + name ||
                        state_name.ends_with("." + name)) {
                        value = state;
                        break;
                    }
                system_.signals[name] = value;
                system_.builder.output(value, name);
            }
            return;
        }
        auto result_width = eir::Type::parse(operation.result_type).width();
        if (opcode == "not")
            return bind(operation, system_.builder.unary("not", operand(arguments[0])));
        if (opcode == "redand" || opcode == "redor" || opcode == "redxor")
            return bind(operation, system_.builder.unary(opcode, operand(arguments[0]), 1));
        if (opcode == "mux") {
            auto when_true = resize(operand(arguments[2]), result_width);
            auto when_false = resize(operand(arguments[1]), result_width);
            return bind(operation, system_.builder.ternary(
                "ite", boolean(operand(arguments[0])),
                when_true, when_false));
        }
        if (opcode == "slice")
            return bind(operation, system_.builder.slice(
                operand(arguments[0]), integer(arguments[1]),
                integer(arguments[2])));
        if (opcode == "zext" || opcode == "sext")
            return bind(operation, system_.builder.extend(
                opcode == "zext" ? "uext" : "sext",
                operand(arguments[0]), result_width));
        if (opcode == "trunc")
            return bind(operation, system_.builder.slice(
                operand(arguments[0]), 0, result_width));
        static const std::unordered_map<std::string, std::string> binary = {
            {"and", "and"}, {"or", "or"}, {"xor", "xor"},
            {"add", "add"}, {"sub", "sub"}, {"mul", "mul"},
            {"shl", "sll"}, {"lshr", "srl"}, {"ashr", "sra"},
            {"eq", "eq"}, {"ne", "neq"}, {"slt", "slt"},
            {"sle", "slte"}, {"ult", "ult"}, {"ule", "ulte"},
            {"concat", "concat"}
        };
        auto found = binary.find(opcode);
        if (found == binary.end())
            throw std::runtime_error("unsupported EIR-to-BTOR2 opcode: " + opcode);
        bind(operation, system_.builder.binary(
            found->second, operand(arguments[0]), operand(arguments[1]),
            result_width));
    }

    void finish_states() {
        for (const auto& [name, state] : states_) {
            auto found = next_states_.find(name);
            system_.builder.next(
                state, found == next_states_.end() ? state : found->second);
        }
    }
};
}

TwoStateTransitionSystem lower_two_state(const eir::Program& program) {
    if (program.modules.empty()) {
        TwoStateTransitionSystem system;
        auto zero = system.builder.constant(1, "0");
        auto initialized = system.builder.state(1, "formal_initialized");
        system.builder.init(initialized, zero);
        system.builder.next(initialized, system.builder.constant(1, "1"));
        system.signals["formal_initialized"] = initialized;
        return system;
    }
    return Lowerer(eir::lowering::flatten(program)).run();
}
}
