/*
 * compiler/eir/lib/IR/Builder.cpp
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

#include "../../include/IR/Builder.h"
#include <stdexcept>

namespace emul::eir {

Builder::Builder(Module& module) {
    set_insertion_point(module);
}

Builder::Builder(Block& block) {
    set_insertion_point(block);
}

void Builder::set_insertion_point(Module& module) {
    module_ = &module;
    operations_ = &module.operations;
}

void Builder::set_insertion_point(Block& block) {
    module_ = nullptr;
    operations_ = &block.operations;
}

Operation& Builder::append(Operation operation) {
    if (!operations_)
        throw std::runtime_error("EIR builder has no insertion point");
    operations_->push_back(std::move(operation));
    return operations_->back();
}

Operation& Builder::create(std::string opcode, std::string operands, std::string result_type) {
    auto result = result_type.empty() ? std::string{} : fresh_value();
    return append({std::move(result), std::move(result_type),
                   std::move(opcode), std::move(operands)});
}

Value& Builder::add_input(std::string name, std::string type) {
    if (!module_)
        throw std::runtime_error("inputs require a module insertion point");
    module_->inputs.push_back({std::move(name), std::move(type)});
    return module_->inputs.back();
}

Value& Builder::add_result(std::string name, std::string type) {
    if (!module_)
        throw std::runtime_error("results require a module insertion point");
    module_->results.push_back({std::move(name), std::move(type)});
    return module_->results.back();
}

State& Builder::add_state(std::string name, std::string type) {
    if (!module_)
        throw std::runtime_error("states require a module insertion point");
    module_->states.push_back({std::move(name), std::move(type)});
    return module_->states.back();
}

std::string Builder::fresh_value() {
    for (;;) {
        auto candidate = "%" + std::to_string(next_value_++);
        bool used = false;
        for (const auto& operation : *operations_)
            for (const auto& result : operation.result_list())
                used |= result == candidate;
        if (!used)
            return candidate;
    }
}
}
