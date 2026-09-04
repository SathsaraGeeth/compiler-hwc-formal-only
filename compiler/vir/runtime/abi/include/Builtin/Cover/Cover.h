#pragma once
#include "../../Core/Value.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace vir::runtime::builtin {
class Cover {
public:
    void sample(std::string bin, const Value& value);
    void hit(std::string bin);
    uint64_t hits(std::string_view bin) const;
    void reset();
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, uint64_t> hits_;
};
}
