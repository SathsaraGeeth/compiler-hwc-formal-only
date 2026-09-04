#pragma once
#include <chrono>
#include <cstdint>
#include <functional>

namespace vir::runtime::builtin {
class Time {
public:
    using Clock = std::chrono::steady_clock;
    using Predicate = std::function<bool()>;
    Time();
    uint64_t now() const;
    void delay(uint64_t nanoseconds) const;
    bool wait_until(const Predicate& condition, uint64_t timeout_ns = 0) const;
    bool wait_edge(const Predicate& signal, bool rising,
                   uint64_t timeout_ns = 0) const;
private:
    Clock::time_point epoch_;
};
}
