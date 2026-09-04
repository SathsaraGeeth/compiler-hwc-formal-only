#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>

namespace vir::runtime::builtin {
class Scheduler {
public:
    using Task = std::function<void()>;
    void add(Task task) { schedule(std::move(task), now_); }
    void schedule(Task task, uint64_t time);
    void run();
    void stop();
    uint64_t now() const noexcept { return now_; }
private:
    struct Entry { uint64_t time, order; Task task; };
    struct Later { bool operator()(const Entry& a, const Entry& b) const {
        return a.time != b.time ? a.time > b.time : a.order > b.order;
    }};
    std::mutex mutex_;
    std::priority_queue<Entry, std::vector<Entry>, Later> tasks_;
    uint64_t now_ = 0, order_ = 0;
    bool stopped_ = false;
};
}
