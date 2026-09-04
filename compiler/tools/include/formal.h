/*
 * compiler/tools/include/formal.h
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
#include <string_view>

namespace emul::tool {
bool run_formal(
    const Session& session,
    FormalAction action,
    std::string_view property);
}
