/*
 * compiler/eir/include/IR/Value.h
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
 * 1. EIR value
 * 2. e.g., %0 : 4s<8> = add %a,%b
 *    - %0 is a value
 * 3. Attrs
 *    - name; e.g. "%0  "
 *    - type; e.g. "4s<8>"
 * 4. Methods
 *    - value_type; convert into name into Type object; e.g. auto t = value.value_type();
 *    - is_ssa; just checks is name start with % (not about the SSA property)
 *    - ==
 */

#pragma once
#include "Type.h"
#include <string>

namespace emul::eir {
struct Value {
    std::string name;
    std::string type;
    Type value_type() const;
    bool is_ssa() const noexcept;
    friend bool operator==(const Value&, const Value&) = default;
};
}
