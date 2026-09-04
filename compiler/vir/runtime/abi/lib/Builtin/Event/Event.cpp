#include "Builtin/Event/Event.h"
#include <chrono>

namespace vir::runtime::builtin {
void Event::trigger() { std::lock_guard lock(mutex_); triggered_ = true; changed_.notify_all(); }
void Event::wait() { std::unique_lock lock(mutex_); changed_.wait(lock, [&] { return triggered_; }); }
bool Event::wait_for(uint64_t ns) { std::unique_lock lock(mutex_); return changed_.wait_for(lock, std::chrono::nanoseconds(ns), [&] { return triggered_; }); }
bool Event::is_triggered() const { std::lock_guard lock(mutex_); return triggered_; }
void Event::reset() { std::lock_guard lock(mutex_); triggered_ = false; }
}
