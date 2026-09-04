#pragma once
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

namespace vir::runtime::builtin {
class Assume {
public:
    void add(bool condition, std::string description = {});
    bool valid() const;
    size_t size() const;
private:
    mutable std::mutex mutex_;
    std::vector<std::pair<bool, std::string>> assumptions_;
};
}
