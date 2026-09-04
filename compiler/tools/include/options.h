/*
 * compiler/tools/include/options.h
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

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace emul::tool {
enum class Target { cpu, fpga };
enum class FormalAction { prove, cover };

struct Inputs {
    std::vector<std::filesystem::path> sources;
    std::vector<std::filesystem::path> libraries;
    std::vector<std::filesystem::path> include_dirs;
    std::vector<std::string> defines;
};

struct FormalMatrixTask {
    std::string name;
    std::string engine;
    std::string backend;
};

struct Session {
    Inputs inputs;
    std::string top;
    std::string timescale;
    Target target = Target::cpu;
    std::string uvm_test;
    uint64_t seed = 1;
    std::string clock;
    std::vector<std::string> clocks;
    std::string reset;
    std::string engine = "auto";
    std::string formal_backend = "auto";
    std::string formal_task = "auto";
    std::string formal_property;
    std::filesystem::path formal_work_directory = "formal-work";
    std::string expected_result = "default";
    std::string cover_trace = "shortest";
    uint64_t formal_timeout_ms = 0;
    bool no_run = false;
    uint32_t max_depth = 20;
    std::vector<FormalMatrixTask> formal_matrix;
};
}
