/* Implements deterministic formal progress logging. */

#include "logger.h"
#include <ostream>

namespace emul::formal {
Logger::Logger(std::ostream& output) : output_(output) {}

void Logger::phase(std::string_view name, std::string_view detail) {
    output_ << "[formal] " << name;
    if (!detail.empty())
        output_ << ": " << detail;
    output_ << '\n';
}

void Logger::result(std::string_view status, std::string_view detail) {
    output_ << "[formal] result: " << status;
    if (!detail.empty())
        output_ << " (" << detail << ")";
    output_ << '\n';
}
}
