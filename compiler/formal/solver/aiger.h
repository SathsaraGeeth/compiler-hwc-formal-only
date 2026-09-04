#pragma once
#include "../engine/engine.h"

namespace emul::formal {
enum class AigerBackend { abc, avy, suprove };
Report run_aiger_backend(const std::filesystem::path& executable,
                         AigerBackend backend,
                         const std::filesystem::path& transition_system,
                         const EngineOptions& options, std::ostream& log);
}
