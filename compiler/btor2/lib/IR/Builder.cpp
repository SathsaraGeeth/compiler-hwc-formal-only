/*
 * compiler/btor2/lib/IR/Builder.cpp
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
#include "../../include/IR/Operation.h"
#include "../../include/IR/Printer.h"
#include "../../include/IR/Verifier.h"

namespace emul::btor2 {

Btor2Value Builder::add(Operation operation) {
    const auto width = operation.width;
    return {module_.add(std::move(operation)), width};
}

Btor2Value Builder::input(uint32_t width, std::string_view name) {
    return add(Operation::input(width, name));
}

Btor2Value Builder::state(uint32_t width, std::string_view name) {
    auto value = add(Operation::state(width, name));
    module_.add_state(value);
    return value;
}

void Builder::init(Btor2Value state, Btor2Value value) {
    module_.init(state, value);
}

Btor2Value Builder::constant(uint32_t width, std::string_view bits) {
    return add(Operation::constant(width, bits));
}

Btor2Value Builder::unary(std::string_view opcode, Btor2Value value,
                          uint32_t result_width) {
    return add(Operation::unary(opcode, value, result_width));
}

Btor2Value Builder::binary(std::string_view opcode, Btor2Value left,
                           Btor2Value right, uint32_t result_width) {
    return add(Operation::binary(opcode, left, right, result_width));
}

Btor2Value Builder::ternary(std::string_view opcode, Btor2Value condition,
                            Btor2Value left, Btor2Value right) {
    return add(Operation::ternary(opcode, condition, left, right));
}

Btor2Value Builder::slice(Btor2Value value, uint32_t offset, uint32_t width) {
    return add(Operation::slice(value, offset, width));
}

Btor2Value Builder::extend(std::string_view opcode, Btor2Value value,
                           uint32_t width) {
    return add(Operation::extend(opcode, value, width));
}

void Builder::next(Btor2Value state, Btor2Value value) {
    module_.next(state, value);
}

void Builder::output(Btor2Value value, std::string_view name) {
    module_.output(value, name);
}

void Builder::constraint(Btor2Value value, std::string_view name) {
    module_.constraint(value, name);
}

void Builder::bad(Btor2Value value, std::string_view name) {
    module_.bad(value, name);
}

void Builder::fair(Btor2Value value, std::string_view name) {
    module_.fair(value, name);
}

void Builder::justice(const std::vector<Btor2Value>& conditions, std::string_view name) {
    module_.justice(conditions, name);
}

Module& Builder::module() noexcept { return module_; }

const Module& Builder::module() const noexcept { return module_; }

void Builder::write(std::ostream& output) const {
    verify(module_);
    print(module_, output);
}

} 
