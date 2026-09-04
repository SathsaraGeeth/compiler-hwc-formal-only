/*
 * compiler/tools/include/dump.h
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

void dump_representation(const Session& session, std::string_view kind,
                         std::string_view argument = {});

}
