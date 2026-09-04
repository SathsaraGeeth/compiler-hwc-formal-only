/*
 * compiler/tools/include/artifacts.h
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
void write_artifacts(const Session& session,
                     const std::filesystem::path& directory);
}
