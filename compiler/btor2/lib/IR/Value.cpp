/*
 * compiler/btor2/lib/IR/Value.cpp
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

namespace emul::btor2 {
Value::operator bool() const noexcept { return node != 0; }
} 
