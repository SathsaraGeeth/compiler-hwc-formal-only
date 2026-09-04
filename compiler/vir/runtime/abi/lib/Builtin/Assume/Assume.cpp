#include "Builtin/Assume/Assume.h"

namespace vir::runtime::builtin {
void Assume::add(bool condition, std::string description) { std::lock_guard lock(mutex_); assumptions_.emplace_back(condition, std::move(description)); }
bool Assume::valid() const { std::lock_guard lock(mutex_); for (auto& item : assumptions_) if (!item.first) return false; return true; }
size_t Assume::size() const { std::lock_guard lock(mutex_); return assumptions_.size(); }
}
