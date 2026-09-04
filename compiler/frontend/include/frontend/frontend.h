/*
 * compiler/frontend/include/frontend/frontend.h
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

#include "frontend/elaborated_design.h"
#include <filesystem>
#include <string>
#include <vector>

namespace emul::frontend {

struct FrontendInput {
    
    std::string top;

    
    std::vector<std::filesystem::path> sources;

    
    std::vector<std::filesystem::path> library_sources;

    
    std::vector<std::filesystem::path> include_dirs;

    
    std::vector<std::string> defines;

    
    std::string timescale;
};

class Frontend {
public:
    
    ElaboratedDesign elaborate(const FrontendInput& input) const;
};
}  
