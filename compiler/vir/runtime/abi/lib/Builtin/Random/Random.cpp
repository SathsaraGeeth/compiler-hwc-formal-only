#include "Builtin/Random/Random.h"
#include <stdexcept>

namespace vir::runtime::builtin {
void Random::seed(uint64_t value) { std::lock_guard lock(mutex_); engine_.seed(value); }
uint64_t Random::integer() { std::lock_guard lock(mutex_); return engine_(); }
int64_t Random::range(int64_t minimum, int64_t maximum) { if (minimum > maximum) throw std::invalid_argument("invalid random range"); std::lock_guard lock(mutex_); return std::uniform_int_distribution<int64_t>(minimum, maximum)(engine_); }
}
