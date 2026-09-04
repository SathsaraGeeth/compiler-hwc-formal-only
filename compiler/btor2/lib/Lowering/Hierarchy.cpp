/*
 * compiler/btor2/lib/Lowering/Hierarchy.cpp
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

#include "Lowering/Hierarchy.h"
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace emul::btor2 {
namespace {
std::string join(const std::vector<std::string>& values) {
    std::string result;
    for (const auto& value : values) {
        if (!result.empty()) result += ", ";
        result += value;
    }
    return result;
}

std::string qualify(const std::string& value, const std::string& path) {
    if (path.empty() || value.empty() || (value.front() != '%' && value.front() != '@'))
        return value;
    return value.substr(0, 1) + path + "." + value.substr(1);
}

std::vector<std::string> call_arguments(const std::string& invocation,
                                        std::string& module_name) {
    auto open = invocation.find('(');
    auto close = invocation.rfind(')');
    if (open == invocation.npos || close == invocation.npos || close < open)
        throw std::runtime_error("malformed EIR instance invocation " + invocation);
    module_name = invocation.substr(0, open);
    if (!module_name.empty() && module_name.front() == '@') module_name.erase(0, 1);
    eir::Operation parser;
    parser.operands = invocation.substr(open + 1, close - open - 1);
    return parser.operand_list();
}

class Flattener {
public:
    explicit Flattener(const eir::Program& program) : program_(program) {}

    eir::Module run() {
        auto* root = program_.root();
        if (!root) throw std::runtime_error("BTOR2 hierarchy lowering requires an EIR root module");
        result_.name = root->name;
        result_.inputs = root->inputs;
        result_.results = root->results;
        std::unordered_map<std::string, std::string> bindings;
        for (const auto& input : root->inputs) bindings[input.name] = input.name;
        inline_module(*root, {}, bindings, true);
        return std::move(result_);
    }

private:
    std::string map(const std::string& value, const std::string& path,
                    const std::unordered_map<std::string, std::string>& bindings) const {
        auto found = bindings.find(value);
        return found == bindings.end() ? qualify(value, path) : found->second;
    }

    std::vector<std::string> inline_module(
        const eir::Module& module, const std::string& path,
        std::unordered_map<std::string, std::string> bindings, bool root) {
        for (const auto& state : module.states) {
            auto renamed = state;
            renamed.name = qualify(state.name, path);
            result_.states.push_back(std::move(renamed));
            bindings[state.name] = qualify(state.name, path);
        }
        std::vector<std::string> outputs;
        for (const auto& operation : module.operations) {
            auto operands = operation.operand_list();
            if (operation.opcode == "instance") {
                if (operands.size() != 2)
                    throw std::runtime_error("malformed EIR instance");
                std::string callee_name;
                auto arguments = call_arguments(operands[1], callee_name);
                auto* callee = program_.find_module(callee_name);
                if (!callee) throw std::runtime_error("unknown EIR module @" + callee_name);
                if (arguments.size() != callee->inputs.size())
                    throw std::runtime_error("EIR instance argument count mismatch for @" + callee_name);
                std::unordered_map<std::string, std::string> child_bindings;
                for (std::size_t index = 0; index < arguments.size(); ++index)
                    child_bindings[callee->inputs[index].name] = map(arguments[index], path, bindings);
                auto instance_name = operands[0];
                if (!instance_name.empty() && instance_name.front() == '@') instance_name.erase(0, 1);
                auto child_path = path.empty() ? instance_name : path + "." + instance_name;
                auto child_outputs = inline_module(*callee, child_path, std::move(child_bindings), false);
                auto results = operation.result_list();
                if (results.size() != child_outputs.size())
                    throw std::runtime_error("EIR instance result count mismatch for @" + callee_name);
                for (std::size_t index = 0; index < results.size(); ++index)
                    bindings[results[index]] = child_outputs[index];
                continue;
            }
            for (auto& operand : operands) operand = map(operand, path, bindings);
            if (operation.opcode == "yield") {
                outputs = std::move(operands);
                if (root) {
                    auto copy = operation;
                    copy.operands = join(outputs);
                    result_.operations.push_back(std::move(copy));
                }
                continue;
            }
            auto copy = operation;
            auto results = operation.result_list();
            for (auto& result : results) {
                auto renamed = qualify(result, path);
                bindings[result] = renamed;
                result = std::move(renamed);
            }
            copy.result = join(results);
            copy.operands = join(operands);
            result_.operations.push_back(std::move(copy));
        }
        return outputs;
    }

    const eir::Program& program_;
    eir::Module result_;
};
}

eir::Module flatten_hierarchy(const eir::Program& program) {
    return Flattener(program).run();
}
} 
