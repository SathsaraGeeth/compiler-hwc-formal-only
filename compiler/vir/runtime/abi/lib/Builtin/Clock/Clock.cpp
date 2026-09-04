#include "Builtin/Clock/Clock.h"
#include <chrono>

namespace vir::runtime::builtin {
Clock::Clock(uint64_t period_ns) : period_ns_(period_ns) {}
Clock::~Clock() { stop(); }
void Clock::start() {
    if (worker_.joinable() || !period_ns_) return;
    worker_ = std::jthread([this](std::stop_token stop) {
        while (!stop.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(period_ns_ / 2));
            const bool next = !value_.load(); value_ = next;
            auto& edge = next ? rising_ : falling_; edge.reset(); edge.trigger();
        }
    });
}
void Clock::stop() { if (worker_.joinable()) { worker_.request_stop(); worker_.join(); } }
void Clock::wait_edge(Edge edge) { auto& event = edge == Edge::rising ? rising_ : falling_; event.reset(); event.wait(); }
}
