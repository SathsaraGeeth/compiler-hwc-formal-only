/*
 * compiler/eir/include/IR/Block.h
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
 * 1. Basic blocks - single entry single exits op sequence
 * 2. Attrs
 *    - name
 *    - operations
 * 4. Methods
 *    - empty
 *    - terminator
 */

#pragma once
#include "Operation.h"
#include <string>
#include <vector>

namespace emul::eir {
struct Block {
    std::string name;
    std::vector<Operation> operations;
    bool empty() const noexcept;
    Operation* terminator() noexcept;
    const Operation* terminator() const noexcept;
};
}
