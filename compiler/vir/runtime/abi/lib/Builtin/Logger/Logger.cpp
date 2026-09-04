#include "Builtin/Logger/Logger.h"
#include <iostream>

namespace vir::runtime::builtin {
Logger::Logger(Sink sink) : sink_(std::move(sink)) { if (!sink_) sink_ = [](Level level, std::string_view message) { std::clog << '[' << int(level) << "] " << message << '\n'; }; }
void Logger::log(Level level, std::string_view message) const { std::lock_guard lock(mutex_); if (level >= level_) sink_(level, message); }
}
