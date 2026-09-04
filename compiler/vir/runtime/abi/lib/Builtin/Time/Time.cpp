#include "Builtin/Time/Time.h"
#include <thread>

namespace vir::runtime::builtin {
Time::Time() : epoch_(Clock::now()) {}
uint64_t Time::now() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - epoch_).count();
}
void Time::delay(uint64_t nanoseconds) const {
    std::this_thread::sleep_for(std::chrono::nanoseconds(nanoseconds));
}
bool Time::wait_until(const Predicate& condition, uint64_t timeout_ns) const {
    const auto deadline = timeout_ns ? Clock::now() + std::chrono::nanoseconds(timeout_ns)
                                     : Clock::time_point::max();
    while (!condition()) {
        if (Clock::now() >= deadline) return false;
        std::this_thread::yield();
    }
    return true;
}
bool Time::wait_edge(const Predicate& signal, bool rising, uint64_t timeout_ns) const {
    const bool initial = signal();
    return wait_until([&] {
        const bool current = signal();
        return rising ? (!initial && current) : (initial && !current);
    }, timeout_ns);
}
}
