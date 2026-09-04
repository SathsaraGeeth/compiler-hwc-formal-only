/*
 * compiler/btor2/include/IR/Value.h
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
 * 1. Typed reference to a BTOR2 operation result.
 * 2. e.g., Value value{4, 8};
 *          node = 4, width = 8
 * 3. Attrs
 *    - node; stable operation or textual BTOR2 node ID
 *    - width; bit vector width of the referenced result
 * 4. Methods
 *    - bool; true when the value references a nonzero node ID
 *    - ==; compares the node ID and width
 * 5. NodeId is the compact ID type used by a materialized Module.
 */

#pragma once
#include <cstdint>

namespace emul::btor2 {

using NodeId = std::uint32_t;

struct Value {
    std::uint64_t node = 0;
    std::uint32_t width = 0;

    explicit operator bool() const noexcept;
    friend bool operator==(const Value&, const Value&) = default;
};

} // namespace emul::btor2
