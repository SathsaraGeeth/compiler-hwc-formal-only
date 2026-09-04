#include "Builtin/UVM/Coverage.h"
#include <algorithm>
#include <iomanip>
#include <ostream>
#include <vector>

namespace vir::runtime::builtin::uvm {
void Coverage::register_bin(std::string bin) { std::lock_guard lock(mutex_); bins_.insert(std::move(bin)); }
void Coverage::sample(std::string bin, const Value& value) {
    register_bin(bin); cover_.sample(std::move(bin), value);
}
double Coverage::percentage() const {
    std::lock_guard lock(mutex_);
    // A model with no registered coverage obligations is complete by
    // definition; this also prevents coverage-driven tests with an optional
    // subscriber from waiting forever.
    if (bins_.empty()) return 100.0;
    std::size_t covered = 0;
    for (const auto& bin : bins_) covered += cover_.hits(bin) != 0;
    return 100.0 * static_cast<double>(covered) / static_cast<double>(bins_.size());
}
void Coverage::report(std::ostream& output) const {
    std::lock_guard lock(mutex_);
    std::vector<std::string> ordered(bins_.begin(), bins_.end());
    std::sort(ordered.begin(), ordered.end());
    std::size_t covered = 0;
    for (const auto& bin : ordered) covered += cover_.hits(bin) != 0;
    const auto percent = ordered.empty() ? 100.0
        : 100.0 * static_cast<double>(covered) / static_cast<double>(ordered.size());
    output << "UVM_COVERAGE " << std::fixed << std::setprecision(2) << percent
           << "% (" << covered << '/' << ordered.size() << " bins)\n";
    for (const auto& bin : ordered)
        output << "  " << bin << " hits=" << cover_.hits(bin) << '\n';
}
void Coverage::reset() {
    std::lock_guard lock(mutex_);
    bins_.clear();
    cover_.reset();
}
} // namespace vir::runtime::builtin::uvm
