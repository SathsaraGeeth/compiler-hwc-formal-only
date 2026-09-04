#pragma once
#include "IR/Module.h"
#include <filesystem>

namespace emul::formal {
struct PropertyJobs {
    std::filesystem::path safety_model;
    std::filesystem::path liveness_model;
};

bool has_parallel_property_jobs(const btor2::Module& module);
PropertyJobs create_property_jobs(
    const btor2::Module& module,
    const std::filesystem::path& work_directory);
}
