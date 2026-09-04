#include "Builtin/UVM/Report.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace vir::runtime::builtin::uvm {
namespace {
const char* name(Severity severity) {
    switch (severity) {
    case Severity::info: return "UVM_INFO";
    case Severity::warning: return "UVM_WARNING";
    case Severity::error: return "UVM_ERROR";
    case Severity::fatal: return "UVM_FATAL";
    }
    return "UVM_UNKNOWN";
}
}

void Report::emit(Severity severity, std::string_view id,
                  std::string_view message, std::int32_t verbosity,
                  std::string_view file, std::int32_t line) {
    {
        std::lock_guard lock(mutex_);
        if (severity == Severity::info && verbosity > verbosity_) return;
        if (severity == Severity::warning) ++warnings_;
        if (severity == Severity::error) ++errors_;
        if (severity == Severity::fatal) ++fatals_;
        std::cout << name(severity) << " [" << id << "] " << message;
        if (!file.empty()) std::cout << " (" << file << ':' << line << ')';
        std::cout << '\n';
    }
    if (severity == Severity::fatal)
        throw std::runtime_error("UVM_FATAL [" + std::string(id) + "] " +
                                 std::string(message));
}

void Report::set_verbosity(std::int32_t verbosity) noexcept {
    std::lock_guard lock(mutex_);
    verbosity_ = verbosity;
}
std::uint64_t Report::warnings() const noexcept { std::lock_guard lock(mutex_); return warnings_; }
std::uint64_t Report::errors() const noexcept { std::lock_guard lock(mutex_); return errors_; }
std::uint64_t Report::fatals() const noexcept { std::lock_guard lock(mutex_); return fatals_; }
void Report::summary(std::ostream& output) const {
    std::lock_guard lock(mutex_);
    output << "UVM_REPORT_SUMMARY warnings=" << warnings_
           << " errors=" << errors_ << " fatals=" << fatals_ << '\n';
}
void Report::reset() noexcept {
    std::lock_guard lock(mutex_);
    warnings_ = errors_ = fatals_ = 0;
    verbosity_ = 200;
}
} // namespace vir::runtime::builtin::uvm
