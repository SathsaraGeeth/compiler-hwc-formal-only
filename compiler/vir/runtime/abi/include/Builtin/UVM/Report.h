#pragma once

#include <cstdint>
#include <iosfwd>
#include <mutex>
#include <string_view>

namespace vir::runtime::builtin::uvm {

enum class Severity : std::uint32_t { info, warning, error, fatal };

class Report {
public:
    void emit(Severity severity, std::string_view id, std::string_view message,
              std::int32_t verbosity, std::string_view file, std::int32_t line);
    void set_verbosity(std::int32_t verbosity) noexcept;
    std::uint64_t warnings() const noexcept;
    std::uint64_t errors() const noexcept;
    std::uint64_t fatals() const noexcept;
    void summary(std::ostream& output) const;
    void reset() noexcept;

private:
    mutable std::mutex mutex_;
    std::int32_t verbosity_ = 200;
    std::uint64_t warnings_ = 0;
    std::uint64_t errors_ = 0;
    std::uint64_t fatals_ = 0;
};

} // namespace vir::runtime::builtin::uvm
