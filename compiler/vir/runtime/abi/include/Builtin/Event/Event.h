#pragma once
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace vir::runtime::builtin {
class Event {
public:
    void trigger();
    void wait();
    bool wait_for(uint64_t nanoseconds);
    bool is_triggered() const;
    void reset();
private:
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    bool triggered_ = false;
};
}
