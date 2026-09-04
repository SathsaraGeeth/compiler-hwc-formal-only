/*
 * compiler/eir/lib/Lowering/Flatten.cpp
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

#include "../../include/Lowering/Flatten.h"
#include <regex>
#include <stdexcept>
#include <unordered_map>

namespace emul::eir::lowering {
namespace {
std::vector<std::string> split(std::string text) {
    std::vector<std::string> result;
    size_t start = 0;
    int depth = 0;
    for (size_t index = 0; index <= text.size(); ++index) {
        if (index < text.size()) {
            depth += text[index] == '(' ? 1 : text[index] == ')' ? -1 : 0;
            if (text[index] != ',' || depth) continue;
        }
        auto item = text.substr(start, index - start);
        auto first = item.find_first_not_of(' '), last = item.find_last_not_of(' ');
        result.push_back(first == item.npos ? "" : item.substr(first, last - first + 1));
        start = index + 1;
    }
    return result;
}

std::vector<std::string> ssa_values(std::string_view text) {
    std::vector<std::string> result;
    static const std::regex pattern(R"(%[A-Za-z_0-9]+)");
    std::string source(text);
    for (std::sregex_iterator current(
             source.begin(), source.end(), pattern), end;
         current != end; ++current)
        result.push_back(current->str());
    return result;
}

void schedule(eir::Module& module) {
    std::unordered_map<std::string, bool> ready;
    for (const auto& input : module.inputs)
        ready[input.name] = true;
    std::vector<eir::Operation> pending;
    std::vector<eir::Operation> terminators;
    for (auto& operation : module.operations) {
        if (operation.opcode == "yield")
            terminators.push_back(std::move(operation));
        else
            pending.push_back(std::move(operation));
    }
    module.operations.clear();
    while (!pending.empty()) {
        bool progress = false;
        for (size_t index = 0; index < pending.size();) {
            bool operands_ready = true;
            for (const auto& operand : ssa_values(pending[index].operands))
                operands_ready &= ready.contains(operand);
            if (!operands_ready) {
                ++index;
                continue;
            }
            for (const auto& result : split(pending[index].result))
                if (!result.empty())
                    ready[result] = true;
            module.operations.push_back(std::move(pending[index]));
            pending.erase(pending.begin() + index);
            progress = true;
        }
        if (!progress)
            throw std::runtime_error(
                "cannot schedule flattened EIR dataflow in @" + module.name);
    }
    for (auto& operation : terminators) {
        for (const auto& operand : ssa_values(operation.operands))
            if (!ready.contains(operand))
                throw std::runtime_error(
                    "undefined flattened EIR value " + operand);
        module.operations.push_back(std::move(operation));
    }
}

class Flattener {
public:
    explicit Flattener(const eir::Program& program) {
        for (const auto& module : program.modules) modules[module.name] = &module;
        if (program.modules.empty()) throw std::runtime_error("cannot flatten empty EIR program");
        auto* root = program.root();
        if (!root)
            throw std::runtime_error("EIR hierarchy has no root module");
        output.name = root->name;
        output.inputs = root->inputs;
        output.results = root->results;
    }
    eir::Module run() {
        std::vector<std::string> inputs;
        for (const auto& input : output.inputs) inputs.push_back(input.name);
        instantiate(*modules.at(output.name), output.name, inputs, true);
        schedule(output);
        return std::move(output);
    }
private:
    eir::Module output;
    std::unordered_map<std::string, const eir::Module*> modules;
    size_t next_value = 0;

    std::string substitute(std::string text,
                           const std::unordered_map<std::string, std::string>& values,
                           const std::unordered_map<std::string, std::string>& states) {
        static const std::regex token(R"((%[A-Za-z_0-9]+)|(@[A-Za-z_0-9.\[\]]+))");
        std::string result;
        size_t cursor = 0;
        for (std::sregex_iterator it(text.begin(), text.end(), token), last; it != last; ++it) {
            result += text.substr(cursor, it->position() - cursor);
            auto value = it->str();
            auto found_value = values.find(value);
            auto found_state = states.find(value);
            result += found_value != values.end() ? found_value->second :
                      found_state != states.end() ? found_state->second : value;
            cursor = it->position() + it->length();
        }
        return result + text.substr(cursor);
    }

    std::vector<std::string> instantiate(const eir::Module& module, const std::string& path,
                                         const std::vector<std::string>& arguments, bool root) {
        std::unordered_map<std::string, std::string> values, states;
        for (size_t index = 0; index < module.inputs.size(); ++index)
            values[module.inputs[index].name] = arguments[index];
        for (const auto& state : module.states) {
            auto renamed = "@" + path + "." + state.name.substr(1);
            states[state.name] = renamed;
            output.states.push_back({renamed, state.type});
        }
        for (const auto& operation : module.operations) {
            if (operation.opcode == "instance")
                continue;
            for (const auto& result : split(operation.result))
                if (!result.empty())
                    values[result] = "%" + std::to_string(next_value++);
        }
        std::vector<std::string> yielded;
        for (const auto& operation : module.operations) {
            if (operation.opcode == "instance") {
                auto first_at = operation.operands.find('@');
                auto comma = operation.operands.find(',', first_at);
                auto second_at = operation.operands.find('@', comma);
                auto open = operation.operands.find('(', second_at);
                auto close = operation.operands.rfind(')');
                auto instance = operation.operands.substr(first_at + 1, comma - first_at - 1);
                auto target = operation.operands.substr(second_at + 1, open - second_at - 1);
                std::vector<std::string> child_arguments;
                for (auto argument : split(operation.operands.substr(open + 1, close - open - 1)))
                    child_arguments.push_back(substitute(argument, values, states));
                auto child_results = instantiate(*modules.at(target), path + "." + instance,
                                                 child_arguments, false);
                auto names = split(operation.result);
                for (size_t index = 0; index < names.size(); ++index)
                    values[names[index]] = child_results[index];
                continue;
            }
            if (operation.opcode == "yield") {
                for (auto value : split(operation.operands))
                    yielded.push_back(substitute(value, values, states));
                if (root) {
                    auto copy = operation;
                    copy.operands.clear();
                    for (size_t index = 0; index < yielded.size(); ++index) {
                        if (index) copy.operands += ", ";
                        copy.operands += yielded[index];
                    }
                    output.operations.push_back(std::move(copy));
                }
                continue;
            }
            auto copy = operation;
            copy.operands = substitute(copy.operands, values, states);
            if (!copy.result.empty()) {
                auto old_results = split(copy.result);
                copy.result.clear();
                for (const auto& old : old_results) {
                    if (!copy.result.empty()) copy.result += ", ";
                    copy.result += values.at(old);
                }
            }
            output.operations.push_back(std::move(copy));
        }
        return yielded;
    }
};
}

Module flatten(const Program& program) { return Flattener(program).run(); }
}
