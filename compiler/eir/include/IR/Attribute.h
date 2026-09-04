/*
 * compiler/eir/include/IR/Attribute.h
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
 * 1. Compile time metadata.
 * 2. Stores meta data like
 *   - source location
 *   - timing constraints
 *   - pipeline hints
 *   - memory placement
 *   - target info.
 * 3. e.g., Attribute {
 *    "eir.source",
 *    "adder.sv"
 *    }
 * 4. Atrs:
 *    - name
 *    - value
 * 4. Methods:
 *    - valid; is valid attr
 *    - ==; compares
 */

#pragma once
#include <string>

namespace emul::eir {
struct Attribute {
    std::string name;
    std::string value;
    bool valid() const noexcept;
    friend bool operator==(const Attribute&, const Attribute&) = default;
};
}
