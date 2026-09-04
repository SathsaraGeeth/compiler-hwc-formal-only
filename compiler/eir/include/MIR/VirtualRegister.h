/*
 * compiler/eir/include/MIR/VirtualRegister.h
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
 * 1. Mutable machine register before physical register allocation.
 * 2. e.g., %sum has the stored name "sum".
 * 3. Attrs
 *    - name; register name without the % prefix
 * 4. Methods
 *    - parse; validates and removes the % prefix
 *    - spelling; returns the name with its % prefix
 *    - valid; checks the stored register name
 *    - ==; compares registers
 * 5. Unlike an EIR SSA value, a MIR register may be assigned repeatedly.
 */

#pragma once
#include <string>
#include <string_view>

namespace emul::mir {
struct VirtualRegister {
    std::string name;
    static VirtualRegister parse(std::string_view spelling);
    std::string spelling() const;
    bool valid() const noexcept;
    friend bool operator==(const VirtualRegister&, const VirtualRegister&) = default;
};
}
