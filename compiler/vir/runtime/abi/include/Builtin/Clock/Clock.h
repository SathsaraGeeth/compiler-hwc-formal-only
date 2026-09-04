#pragma once
#include "../Event/Event.h"
#include <atomic>
#include <cstdint>
#include <thread>

namespace vir::runtime::builtin {
class Clock {
public:
    enum class Edge { rising, falling };
    explicit Clock(uint64_t period_ns);
    ~Clock();
    void start();
    void stop();
    void wait_edge(Edge edge);
    bool value() const noexcept { return value_.load(); }
private:
    uint64_t period_ns_;
    std::atomic<bool> value_{false};
    std::jthread worker_;
    Event rising_, falling_;
};
}
