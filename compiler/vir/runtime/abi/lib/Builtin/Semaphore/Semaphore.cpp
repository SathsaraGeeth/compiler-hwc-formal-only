#include "Builtin/Semaphore/Semaphore.h"

namespace vir::runtime::builtin {
Semaphore::Semaphore(size_t count) : count_(count) {}
void Semaphore::acquire() { std::unique_lock lock(mutex_); changed_.wait(lock, [&] { return count_ != 0; }); --count_; }
bool Semaphore::try_acquire() { std::lock_guard lock(mutex_); if (!count_) return false; --count_; return true; }
void Semaphore::release(size_t count) { std::lock_guard lock(mutex_); count_ += count; changed_.notify_all(); }
}
