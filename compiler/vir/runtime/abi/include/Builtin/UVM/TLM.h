#pragma once

#include "Object/ObjectStore.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace vir::runtime::builtin::uvm {

class TLM {
public:
    explicit TLM(ObjectStore& objects) : objects_(objects) {}
    using Subscriber = std::function<void(const Value&)>;
    void connect(std::string port, std::string endpoint);
    void subscribe(std::string endpoint, Subscriber callback);
    std::vector<std::string> write(std::string_view port, Value value);
    void fifo_push(std::string_view fifo, Value value);
    std::optional<Value> fifo_try_get(std::string_view fifo);
    void reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<std::string>> connections_;
    std::unordered_map<std::string, std::vector<Subscriber>> subscribers_;
    ObjectStore& objects_;
    std::unordered_map<std::string, ObjectHandle> fifos_;
};
} // namespace vir::runtime::builtin::uvm
