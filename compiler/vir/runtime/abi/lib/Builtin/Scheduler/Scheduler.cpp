#include "Builtin/Scheduler/Scheduler.h"

namespace vir::runtime::builtin {
void Scheduler::schedule(Task task, uint64_t time) { std::lock_guard lock(mutex_); tasks_.push({time, order_++, std::move(task)}); }
void Scheduler::run() {
    stopped_ = false;
    while (!stopped_) {
        Task task;
        { std::lock_guard lock(mutex_); if (tasks_.empty()) break; auto entry = tasks_.top(); tasks_.pop(); now_ = entry.time; task = std::move(entry.task); }
        task();
    }
}
void Scheduler::stop() { stopped_ = true; }
}
