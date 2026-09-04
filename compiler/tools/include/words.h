/*
 * compiler/tools/include/words.h
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

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace emul::tool {
std::vector<std::string> split_words(std::string_view text);
}
