#pragma once

#include "../engine/engine.h"
#include <filesystem>
#include <string_view>

namespace emul::formal {
std::string_view result_name(Result) noexcept;
void write_task_artifacts(const Report&, const EngineOptions&,
                          std::string_view property,
                          const std::filesystem::path& model);
}
