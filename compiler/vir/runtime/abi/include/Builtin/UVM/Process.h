#pragma once

#include "Object/ObjectStore.h"
#include <cstdint>
#include <functional>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vir::runtime::builtin::uvm {

// Cooperative process state is deliberately stored in ObjectStore.  The
// scheduler owns only callbacks; program counters, waits and spilled locals
// remain ABI-visible and survive cloning/serialization of the process object.
class Process {
public:
    enum class Status : std::uint64_t { ready, waiting_time, waiting_event, done };
    using Step = std::function<void(ObjectHandle)>;

    explicit Process(ObjectStore& objects) : objects_(objects) {}

    ObjectHandle create(ObjectHandle owner, Step step);
    void register_step(std::string name, Step step);
    ObjectHandle spawn(std::string_view name, ObjectHandle owner);
    std::size_t run();
    bool drain(std::size_t max_steps = 1000000);
    void advance(std::uint64_t time);
    void notify(std::string_view event);
    void reset();

    void set_pc(ObjectHandle process, std::uint64_t pc);
    std::uint64_t pc(ObjectHandle process) const;
    void store(ObjectHandle process, std::string_view name, Value value);
    Value load(ObjectHandle process, std::string_view name) const;
    void wait_time(ObjectHandle process, std::uint64_t delay);
    void wait_event(ObjectHandle process, std::string_view event);
    void complete(ObjectHandle process);
    Status status(ObjectHandle process) const;
    ObjectHandle owner(ObjectHandle process) const;
    ObjectHandle normalize_owner(ObjectHandle possible_process) const;
    std::uint64_t now() const noexcept { return now_; }
    std::size_t size() const noexcept;

private:
    void set_status(ObjectHandle, Status);
    std::uint64_t integer(ObjectHandle, std::string_view) const;

    ObjectStore& objects_;
    mutable std::mutex mutex_;
    std::unordered_map<std::uint64_t, Step> steps_;
    std::unordered_map<std::string, Step> registered_;
    std::vector<ObjectHandle> order_;
    std::uint64_t now_ = 0;
};

} // namespace vir::runtime::builtin::uvm
