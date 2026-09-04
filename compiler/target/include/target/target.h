/*
 * compiler/target/include/target/target.h
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
 * 1. Defines the target-description contract used by EIR code gen
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace emul::target {
struct ValueType {
    uint32_t width = 1;
    bool four_state = true;
};

class Target {
public:
    virtual ~Target() = default;

    /*
     * Q: How many architectural registers can the code generator address?
     * A: Return the target register file size
     */
    virtual uint8_t register_count() const = 0;

    /*
     * Q: Which registers may the allocator use for ordinary temporaries?
     * A: Return the first allocatable register; lower registers are reserved
     */
    virtual uint8_t first_allocatable_register() const = 0;

    /*
     * Q: Does the target implement this EIR operation?
     * A: Return its machine opcode, or no value when it is unsupported
     */
    virtual std::optional<uint8_t> opcode(std::string_view operation) const = 0;

    /*
     * Q: How is one selected machine instruction represented in bits?
     * A: Pack the opcode, registers, value type, and immediate into a word
     */
    virtual uint32_t encode(uint8_t opcode, uint8_t destination, uint8_t source1,
                            uint8_t source2, uint8_t source3, ValueType type,
                            uint32_t immediate) const = 0;
};
} // namespace emul::target
