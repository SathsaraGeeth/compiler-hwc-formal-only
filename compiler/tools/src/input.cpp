/*
 * compiler/tools/src/input.cpp
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

#include "input.h"
#include "frontend/frontend.h"
#include <algorithm>

namespace emul::tool {
namespace {
void append_unique(std::vector<std::filesystem::path>& values,
                   std::filesystem::path value) {
    if (std::find(values.begin(), values.end(), value) == values.end())
        values.push_back(std::move(value));
}
}

frontend::FrontendInput frontend_input(const Session& session) {
    frontend::FrontendInput result;
    result.top = session.top;
    result.timescale = session.timescale;
    result.sources = session.inputs.sources;
    result.library_sources = session.inputs.libraries;
    result.include_dirs = session.inputs.include_dirs;
    result.defines = session.inputs.defines;
    append_unique(result.include_dirs, HWC_RUNTIME_INCLUDE);
    if (!session.uvm_test.empty()) {
        append_unique(result.include_dirs, HWC_UVM_INCLUDE);
        append_unique(result.library_sources, HWC_UVM_SOURCE);
    }
    return result;
}
}
