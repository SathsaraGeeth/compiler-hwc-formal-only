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
 * 1. Functions
 * 2. for future CFG support
 * 3. TODO: not sure whether this need or not for EIR
 *          current EIR spec dont have CFG concept
 *          probably wont
 */

#pragma once
#include "Region.h"
#include "Value.h"
#include <string>
#include <vector>

namespace emul::eir {
struct Function {
    std::string name;
    std::vector<Value> inputs;
    std::vector<Value> results;
    Region body;
    Block* entry() noexcept;
    const Block* entry() const noexcept;
};
}
