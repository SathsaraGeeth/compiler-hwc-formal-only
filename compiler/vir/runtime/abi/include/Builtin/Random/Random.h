#pragma once
#include <cstdint>
#include <mutex>
#include <random>

namespace vir::runtime::builtin {
class Random {
public:
    explicit Random(uint64_t seed = 1) : engine_(seed) {}
    void seed(uint64_t value);
    uint64_t integer();
    int64_t range(int64_t minimum, int64_t maximum);
private:
    std::mutex mutex_;
    std::mt19937_64 engine_;
};
}
