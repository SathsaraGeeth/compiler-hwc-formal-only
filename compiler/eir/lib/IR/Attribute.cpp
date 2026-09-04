/*
 * compiler/eir/lib/IR/Attribute.cpp
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

#include "../../include/IR/Attribute.h"

namespace emul::eir {
bool Attribute::valid() const noexcept {
    return !name.empty();
}
}
