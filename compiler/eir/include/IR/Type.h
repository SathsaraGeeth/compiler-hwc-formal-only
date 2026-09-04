/*
 * compiler/eir/include/IR/Types.h
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
 * 1. EIR types: as of now 4s<N>, 2s<N>
 * 2. e.g., 4s<32> means 4 state 32 wide SSA value
 * 3. Attrs
 *    - spelling; textual representation e.g. "2s<5>"
 * 4. Methods
 *    - parse; constructor like e.g. auto type = Type::parse("4s<8>")
 *    - bit_vecotor; e.g. auto t = Type::bit_vector(8, true) - creates type 4s
 *    - valid
 *    - two_state/four_state
 *    - width
 *    - ==
 */

#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace emul::eir {
struct Type {
    std::string spelling;
    static Type parse(std::string_view spelling);
    static Type bit_vector(uint32_t width, bool four_state = true);
    bool valid() const noexcept;
    bool two_state() const noexcept;
    bool four_state() const noexcept;
    uint32_t width() const;
    friend bool operator==(const Type&, const Type&) = default;
};
}
