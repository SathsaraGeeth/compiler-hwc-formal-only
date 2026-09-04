/*
 * compiler/eir/lib/IR/Verifier.cpp
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

#include "../../include/IR/Verifier.h"
#include <array>
#include <regex>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

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

std::vector<std::string> values(std::string_view text) {
    std::vector<std::string> result;
    static const std::regex pattern(R"(%[A-Za-z_0-9]+)");
    std::string source(text);
    for (std::sregex_iterator it(source.begin(), source.end(), pattern), last; it != last; ++it)
        result.push_back(it->str());
    return result;
}

std::vector<std::string> results(std::string_view text) {
    return values(text);
}
}

void verify(const Program& program) {
    std::unordered_map<std::string, const Module*> modules;
    for (const auto& module : program.modules)
        if (!modules.emplace(module.name, &module).second)
            throw std::runtime_error("duplicate EIR module @" + module.name);
    for (const auto& module : program.modules) {
        if (module.name.empty()) throw std::runtime_error("empty EIR module name");
        std::unordered_set<std::string> defined;
        std::unordered_set<std::string> available;
        std::unordered_set<std::string> states;
        for (const auto& input : module.inputs) {
            input.value_type();
            if (!input.is_ssa())
                throw std::runtime_error("invalid EIR input " + input.name + " in @" + module.name);
            if (!defined.insert(input.name).second)
                throw std::runtime_error(
                    "duplicate EIR input " + input.name + " in @" + module.name);
            available.insert(input.name);
        }
        for (const auto& result : module.results) {
            if (result.name.empty())
                throw std::runtime_error(
                    "empty EIR result name in @" + module.name);
            result.value_type();
        }
        for (const auto& state : module.states) {
            state.value_type();
            if (state.name.empty() || state.name.front() != '@')
                throw std::runtime_error("invalid EIR state " + state.name + " in @" + module.name);
            if (!states.insert(state.name).second)
                throw std::runtime_error("duplicate state " + state.name + " in @" + module.name);
        }
        bool yielded = false;
        for (const auto& operation : module.operations)
            for (const auto& result : results(operation.result))
                available.insert(result);
        for (size_t operation_index = 0;
             operation_index < module.operations.size(); ++operation_index) {
            const auto& operation = module.operations[operation_index];
            if (!known(operation.opcode))
                throw std::runtime_error(
                    "unknown EIR operation " + operation.opcode + " in @" +
                    module.name);
            for (const auto& attribute : operation.attributes)
                if (!attribute.valid())
                    throw std::runtime_error(
                        "invalid EIR attribute in @" + module.name);
            if (!operation.result_type.empty()) operation.type();
            for (const auto& operand : values(operation.operands))
                if (!defined.contains(operand) &&
                    !(operation.opcode == "instance" &&
                      available.contains(operand)))
                    throw std::runtime_error(
                        "undefined EIR value " + operand + " in @" + module.name);
            if (operation.opcode == "state_read" || operation.opcode == "state_write") {
                auto at = operation.operands.find('@');
                auto comma = operation.operands.find(',', at);
                auto state = operation.operands.substr(at, comma - at);
                if (!states.contains(state))
                    throw std::runtime_error(
                        "undefined EIR state " + state + " in @" + module.name);
            }
            if (operation.opcode == "instance") {
                auto second_at = operation.operands.find('@', operation.operands.find('@') + 1);
                auto open = operation.operands.find('(', second_at);
                auto target = operation.operands.substr(
                    second_at + 1, open - second_at - 1);
                auto found = modules.find(target);
                if (found == modules.end())
                    throw std::runtime_error("undefined EIR module @" + target);
                if (results(operation.result).size() != found->second->results.size())
                    throw std::runtime_error("wrong instance result count for @" + target);
            }
            for (const auto& result : results(operation.result))
                if (!defined.insert(result).second)
                    throw std::runtime_error(
                        "duplicate EIR value " + result + " in @" + module.name);
            if (operation.opcode == "yield") {
                if (yielded) throw std::runtime_error("multiple yields in @" + module.name);
                if (operation_index + 1 != module.operations.size())
                    throw std::runtime_error("yield is not last in @" + module.name);
                auto yield_count = values(operation.operands).size();
                if (yield_count != module.results.size())
                    throw std::runtime_error("wrong yield result count in @" + module.name +
                                             ": got " + std::to_string(yield_count) +
                                             ", expected " + std::to_string(module.results.size()));
                yielded = true;
            }
        }
        if (!yielded) throw std::runtime_error("missing yield in @" + module.name);
    }
}
}
