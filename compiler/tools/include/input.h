/*
 * compiler/tools/include/input.h
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

namespace emul::frontend {
struct FrontendInput;
}

namespace emul::tool {
frontend::FrontendInput frontend_input(const Session& session);
}
