#pragma once
#include <mutex>

namespace vir::runtime::builtin {
class Mutex {
public:
    void lock() { mutex_.lock(); }
    void unlock() { mutex_.unlock(); }
    bool try_lock() { return mutex_.try_lock(); }
private:
    std::mutex mutex_;
};
}
