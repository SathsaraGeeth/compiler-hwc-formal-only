/*
 * compiler/eir/include/IR/Region.h
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
 * 1. A group of Basic blocks
 * 2. e.g., %3 : 4s<8> = add %a,%b
 * 3. Attrs
 *    - blocks
 * 4. Methods
 *    - entry
 *    - find_block
 */

#pragma once
#include "Block.h"
#include <string_view>
#include <vector>

namespace emul::eir {
struct Region {
    std::vector<Block> blocks;
    Block* entry() noexcept;
    const Block* entry() const noexcept;
    Block* find_block(std::string_view name) noexcept;
    const Block* find_block(std::string_view name) const noexcept;
};
}
