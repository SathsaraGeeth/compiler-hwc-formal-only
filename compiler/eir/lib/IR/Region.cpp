/*
 * compiler/eir/lib/IR/Region.cpp
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

#include "../../include/IR/Region.h"
#include <algorithm>

namespace emul::eir {

Block* Region::entry() noexcept {
    return blocks.empty() ? nullptr : &blocks.front();
}

const Block* Region::entry() const noexcept {
    return blocks.empty() ? nullptr : &blocks.front();
}

Block* Region::find_block(std::string_view name) noexcept {
    auto found = std::find_if(blocks.begin(), blocks.end(),
              [&](const Block& block) { return block.name == name; });
    return found == blocks.end() ? nullptr : &*found;
}

const Block* Region::find_block(std::string_view name) const noexcept {
    auto found = std::find_if(blocks.begin(), blocks.end(),
              [&](const Block& block) { return block.name == name; });
    return found == blocks.end() ? nullptr : &*found;
}
}
