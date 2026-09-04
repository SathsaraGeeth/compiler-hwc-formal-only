/* Runs a BTOR2 transition system through the wrapped Pono model checker. */

#pragma once
#include "../engine/engine.h"
#include <filesystem>
#include <iosfwd>

namespace emul::formal {
Report run_pono(
    const std::filesystem::path& executable,
    const std::filesystem::path& transition_system,
    const EngineOptions& options,
    std::ostream& log);
}
