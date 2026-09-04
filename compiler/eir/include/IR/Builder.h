/*
 * compiler/eir/include/IR/Builder.h
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
 * 1. EIR construction API.
 * 2. Provides a convinet way to construct EIR IR
 *    without managing underlying data structures
 *    tho this layer does not do the construction itself
 * 3. e.g.
 *    instead of;
 *
 *      Operation op;
 *      op.result="%0";
 *      op.opcode="add";
 *      op.operands="%a, %b";
 *
 *    just;
 *      Builder builder(module);  // explict prevents Builder builder = module;
 *      auto& sum = builder.create("add", "%a, %b", "4s<8>");
 * 
 */


#pragma once
#include "Block.h"
#include "Module.h"
#include <string_view>
#include <utility>

namespace emul::eir {
class Builder {
public:
    explicit Builder(Module& module);
    explicit Builder(Block& block);
    void set_insertion_point(Module& module);
    void set_insertion_point(Block& block);
    Operation& append(Operation operation);
    Operation& create(std::string opcode, std::string operands = {},
                      std::string result_type = {});
    Value& add_input(std::string name, std::string type);
    Value& add_result(std::string name, std::string type);
    State& add_state(std::string name, std::string type);
private:
    std::string fresh_value();  // creates %0, %1, ... automatically
    Module* module_ = nullptr;
    std::vector<Operation>* operations_ = nullptr;
    size_t next_value_ = 0;
};
}
