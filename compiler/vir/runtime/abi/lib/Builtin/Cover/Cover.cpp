#include "Builtin/Cover/Cover.h"

namespace vir::runtime::builtin {
void Cover::sample(std::string bin, const Value& value) { if (!std::holds_alternative<std::monostate>(value)) hit(std::move(bin)); }
void Cover::hit(std::string bin) { std::lock_guard lock(mutex_); ++hits_[std::move(bin)]; }
uint64_t Cover::hits(std::string_view bin) const { std::lock_guard lock(mutex_); auto found = hits_.find(std::string(bin)); return found == hits_.end() ? 0 : found->second; }
void Cover::reset() { std::lock_guard lock(mutex_); hits_.clear(); }
}
