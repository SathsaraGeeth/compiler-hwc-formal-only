/*
 * compiler/tools/include/file_list.h
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

#include "options.h"
#include <filesystem>

namespace emul::tool {
void read_file_list(Session& session, const std::filesystem::path& path);
}
