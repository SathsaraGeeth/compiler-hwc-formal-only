/*
 * compiler/eir/lib/IR/Function.cpp
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

#include "../../include/IR/Function.h"

namespace emul::eir {

Block* Function::entry() noexcept {
    return body.entry();
}

const Block* Function::entry() const noexcept {
    return body.entry();
}
}
