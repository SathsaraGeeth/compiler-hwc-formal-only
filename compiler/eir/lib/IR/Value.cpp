/*
 * compiler/eir/lib/IR/Value.cpp
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

#include "../../include/IR/Value.h"

namespace emul::eir {

Type Value::value_type() const {
    return Type::parse(type);
}

bool Value::is_ssa() const noexcept {
    return !name.empty() && name.front() == '%';
}
}
