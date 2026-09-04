/*
 * compiler/eir/lib/IR/Block.cpp
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

#include "../../include/IR/Block.h"

namespace emul::eir {

bool Block::empty() const noexcept {
    return operations.empty();
}

Operation* Block::terminator() noexcept {
    return !operations.empty() && operations.back().is_terminator()
           ? &operations.back() : nullptr;
}

const Operation* Block::terminator() const noexcept {
    return !operations.empty() && operations.back().is_terminator()
           ? &operations.back() : nullptr;
}

}
