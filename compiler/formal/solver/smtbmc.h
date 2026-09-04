#pragma once
#include "../engine/engine.h"

namespace emul::formal {
Report run_smtbmc(const std::filesystem::path& executable,
                  const std::filesystem::path& transition_system,
                  const EngineOptions& options, std::ostream& log);
}
