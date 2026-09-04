/*
 * compiler/tools/include/runtime.h
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

namespace emul::tool {
bool simulate(const Session& session);
bool emulate(const Session& session);
}
