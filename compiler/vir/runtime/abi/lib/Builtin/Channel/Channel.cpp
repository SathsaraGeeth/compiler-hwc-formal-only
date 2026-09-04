#include "Builtin/Channel/Channel.h"

namespace vir::runtime::builtin {
void Channel::send(Value value) { std::lock_guard lock(mutex_); values_.push_back(std::move(value)); available_.notify_one(); }
Value Channel::receive() { std::unique_lock lock(mutex_); available_.wait(lock, [&] { return !values_.empty(); }); auto value = std::move(values_.front()); values_.pop_front(); return value; }
std::optional<Value> Channel::try_receive() { std::lock_guard lock(mutex_); if (values_.empty()) return {}; auto value = std::move(values_.front()); values_.pop_front(); return value; }
std::optional<Value> Channel::peek() const { std::lock_guard lock(mutex_); if (values_.empty()) return {}; return values_.front(); }
size_t Channel::size() const { std::lock_guard lock(mutex_); return values_.size(); }
}
