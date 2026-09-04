#pragma once
#include <functional>

namespace vir::runtime::builtin {
class Property {
public:
    using Predicate = std::function<bool()>;
    explicit Property(Predicate predicate) : predicate_(std::move(predicate)) {}
    bool evaluate() const { return !enabled_ || predicate_(); }
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool enabled() const { return enabled_; }
private:
    Predicate predicate_;
    bool enabled_ = true;
};
}
