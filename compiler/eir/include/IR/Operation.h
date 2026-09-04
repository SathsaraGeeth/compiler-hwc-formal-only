/*
 * compiler/eir/include/IR/Operation.h
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
 * 1. Instruction
 * 2. e.g., %3 : 4s<8> = add %a,%b
 * 3. Attrs
 *    - result; e.g. "%3"
 *    - result_type; e.g. "4s<8>"
 *    - opcode; e.g. "add"
 *    - operands; e.g. "%a, %b"
 * 4. Methods
 *    - has_result; e.g. %0 add has result but yield does not
 *    - is_terminator; e.g. yield/return - is this end of an block?
 *    - has_side_effects; does execution this change something other than
 *                        just producing the result
 *                        e.g. Pure: add, mul has no side effects, 
 *                        effect: state_write, instance changes hardware
 *                        (state_write @count, %new_count changes @count state)
 *    - type; e.g. 4s as a Type object
 */

#pragma once
#include "Attribute.h"
#include "Type.h"
#include <string>
#include <vector>

namespace emul::eir {
struct Operation {
    std::string result;
    std::string result_type;
    std::string opcode;
    std::string operands;
    std::vector<Attribute> attributes;
    std::vector<std::string> operand_list() const;
    std::vector<std::string> result_list() const;
    bool has_result() const noexcept;
    bool is_terminator() const noexcept;
    bool has_side_effects() const noexcept;
    Type type() const;
};
}
