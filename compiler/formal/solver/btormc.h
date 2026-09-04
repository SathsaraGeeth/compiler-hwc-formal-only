#pragma once

#include "../engine/engine.h"
#include <filesystem>
#include <iosfwd>

namespace emul::formal {
Report run_btormc(const std::filesystem::path& executable,
                  const std::filesystem::path& transition_system,
                  const EngineOptions& options, std::ostream& log);
}
