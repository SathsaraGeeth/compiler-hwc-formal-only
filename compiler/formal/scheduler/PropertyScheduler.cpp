#include "PropertyScheduler.h"
#include "IR/Printer.h"
#include "IR/Verifier.h"
#include "Transform/IR/PropertyPartition.h"
#include <fstream>
#include <stdexcept>

namespace emul::formal {
namespace {
void write_model(const btor2::Module& module,
                 const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    if (!output)
        throw std::runtime_error("cannot write formal partition: " +
                                 path.string());
    btor2::verify(module);
    btor2::print(module, output);
}
}

bool has_parallel_property_jobs(const btor2::Module& module) {
    return btor2::transform::has_properties(
               module, btor2::transform::PropertyPartition::safety) &&
           btor2::transform::has_properties(
               module, btor2::transform::PropertyPartition::liveness);
}

PropertyJobs create_property_jobs(
    const btor2::Module& module,
    const std::filesystem::path& work_directory) {
    PropertyJobs jobs{
        work_directory / "safety/model/low_opt.btor2",
        work_directory / "liveness/model/low_opt.btor2"};
    write_model(
        btor2::transform::select_properties(
            module, btor2::transform::PropertyPartition::safety),
        jobs.safety_model);
    write_model(
        btor2::transform::select_properties(
            module, btor2::transform::PropertyPartition::liveness),
        jobs.liveness_model);
    return jobs;
}
}
