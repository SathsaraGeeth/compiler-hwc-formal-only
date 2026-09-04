#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>

namespace vir::runtime::builtin {
class Process {
public:
    enum class State { created, running, stopped, completed, failed };
    using Function = std::function<void(std::stop_token)>;
    explicit Process(Function function);
    ~Process();
    void spawn();
    void join();
    void kill();
    State state() const noexcept { return state_.load(); }
    static void yield() { std::this_thread::yield(); }
private:
    Function function_;
    std::jthread thread_;
    std::atomic<State> state_{State::created};
};
}
