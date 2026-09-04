#pragma once
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace vir::runtime::builtin {
class Semaphore {
public:
    explicit Semaphore(size_t count);
    void acquire();
    bool try_acquire();
    void release(size_t count = 1);
private:
    std::mutex mutex_;
    std::condition_variable changed_;
    size_t count_;
};
}
