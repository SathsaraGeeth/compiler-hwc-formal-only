/*
 * compiler/btor2/include/Analysis/Analysis.h
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
 * Describes analysis summaries and execution planning for a BTOR2 module
 */

#pragma once

#include "IR/Module.h"
#include <cstdint>

namespace emul::btor2 {

struct ModelProfile {
    std::uint64_t nodes = 0;
    std::uint64_t inputs = 0;
    std::uint64_t input_bits = 0;
    std::uint64_t states = 0;
    std::uint64_t state_bits = 0;
    std::uint64_t initialized_states = 0;
    std::uint64_t next_states = 0;
    std::uint64_t outputs = 0;
    std::uint64_t constraints = 0;
    std::uint64_t bad_properties = 0;
    std::uint64_t covers = 0;
    std::uint64_t fairness = 0;
};

struct ExecutionPlan {
    std::uint32_t workers = 1;
    bool parallel = false;
};

ModelProfile analyze(const Module& module);
ExecutionPlan plan(
    const ModelProfile& profile,
    std::uint32_t independent_jobs,
    std::uint32_t requested_workers = 0);

} 
