#pragma once
#include "../../Core/Value.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace vir::runtime::builtin {
class Channel {
public:
    void send(Value value);
    Value receive();
    std::optional<Value> try_receive();
    std::optional<Value> peek() const;
    size_t size() const;
private:
    mutable std::mutex mutex_;
    std::condition_variable available_;
    std::deque<Value> values_;
};
}
