/*
 * compiler/target/include/target/emul/emul_target.h
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
 * 1. Declares the FPGA emulator target
 */

#pragma once

#include "target/target.h"

namespace emul::target::emulator {
class EmulatorTarget final : public Target {
public:
    /* Q: How many registers does the emulator ISA expose? */
    /* A: Thirty two five bit register numbers, x0 through x31 */
    uint8_t register_count() const override;

    /* Q: Which emulator register is available first to the allocator? */
    /* A: x1; x0 is reserved by convention. */
    uint8_t first_allocatable_register() const override;

    /* Q: Which opcode implements an EIR operation on the emulator? */
    /* A: Return the fixed opcode table entry, or no value if unsupported */
    std::optional<uint8_t> opcode(std::string_view operation) const override;

    /* Q: How are emulator instruction fields packed? */
    /* A: Pack them into the ISA's 32bit ZERO, R4, R2I, or RI format */
    uint32_t encode(uint8_t opcode, uint8_t destination, uint8_t source1,
                    uint8_t source2, uint8_t source3, ValueType type,
                    uint32_t immediate) const override;
};

const EmulatorTarget& get_target();
} // namespace emul::target::emulator
