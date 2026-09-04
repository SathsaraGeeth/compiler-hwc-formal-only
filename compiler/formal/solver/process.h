/* Executes one tool without a shell and captures its merged diagnostic log. */

#pragma once
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>

namespace emul::formal {
struct ProcessResult {
    int status = -1;
    bool timed_out = false;
    std::chrono::milliseconds elapsed{0};
    std::string output;
};

struct ProcessOptions {
    std::chrono::milliseconds timeout{0};
    std::chrono::milliseconds terminate_grace{2000};
    std::filesystem::path working_directory;
};

ProcessResult run_process(
    const std::filesystem::path& executable,
    const std::vector<std::string>& arguments,
    const ProcessOptions& options = {});
}
