#include "Builtin/UVM/Process.h"
#include "Builtin/UVM/UVM.h"
#include "ABI/Runtime.h"
#include <algorithm>
#include <climits>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <iostream>

namespace vir::runtime::builtin::uvm {
namespace {
constexpr const char* process_type = "uvm_process_state";
std::string local(std::string_view name) { return "local." + std::string(name); }
}

ObjectHandle Process::create(ObjectHandle owner, Step step) {
    if (!step) throw std::invalid_argument("null UVM process step");
    auto process = objects_.create(process_type);
    objects_.store(process, "pc", std::uint64_t{0});
    objects_.store(process, "status", std::uint64_t(Status::ready));
    objects_.store(process, "wake_time", std::uint64_t{0});
    objects_.store(process, "event", std::string{});
    objects_.store(process, "owner", owner);
    std::lock_guard lock(mutex_);
    steps_.emplace(process.value, std::move(step));
    order_.push_back(process);
    return process;
}
void Process::register_step(std::string name, Step step) {
    if (name.empty() || !step) throw std::invalid_argument("invalid UVM process step");
    std::lock_guard lock(mutex_);
    registered_.insert_or_assign(std::move(name), std::move(step));
}
ObjectHandle Process::spawn(std::string_view name, ObjectHandle owner) {
    owner = normalize_owner(owner);
    Step step;
    {
        std::lock_guard lock(mutex_);
        auto found = registered_.find(std::string(name));
        if (found == registered_.end())
            throw std::runtime_error("unknown UVM process step " + std::string(name));
        step = found->second;
        for (auto handle : order_) {
            auto stored_owner = objects_.load(handle, "owner");
            auto stored_name = objects_.load(handle, "name");
            if (!stored_owner.ready() || !stored_name.ready()) continue;
            if (std::get<ObjectHandle>(stored_owner.value()) == owner &&
                std::get<std::string>(stored_name.value()) == name &&
                status(handle) != Status::done)
                return handle;
        }
    }
    auto process = create(owner, std::move(step));
    objects_.store(process, "name", std::string(name));
    return process;
}

std::size_t Process::run() {
    // A delta-cycle snapshot prevents a process that yields at the current
    // time from starving its peers. Newly spawned processes run next delta.
    std::vector<std::pair<ObjectHandle, Step>> ready;
    {
        std::lock_guard lock(mutex_);
        for (auto handle : order_) {
            auto found = steps_.find(handle.value);
            if (found != steps_.end() && status(handle) == Status::ready)
                ready.emplace_back(handle, found->second);
        }
    }
    for (auto& [handle, step] : ready)
        if (status(handle) == Status::ready) {
            if (std::getenv("HWC_TRACE_UVM")) {
                auto name = objects_.load(handle, "name");
                if (name.ready())
                    std::cerr << "UVM_PROCESS_STEP name="
                              << std::get<std::string>(name.value())
                              << " pc=" << pc(handle) << '\n';
            }
            step(handle);
        }
    vir::runtime::abi::flush_nba();
    return ready.size();
}

bool Process::drain(std::size_t max_steps) {
    bool objection_seen = false;
    for (std::size_t iteration = 0; iteration < max_steps; ++iteration) {
        auto ran = run();
        objection_seen |= UVM::objection_count() != 0;
        if (objection_seen && UVM::objection_count() == 0) {
            std::vector<ObjectHandle> handles;
            { std::lock_guard lock(mutex_); handles = order_; }
            for (auto handle : handles)
                if (status(handle) != Status::done) complete(handle);
            return true;
        }
        if (ran) continue;
        std::vector<ObjectHandle> handles;
        { std::lock_guard lock(mutex_); handles = order_; }
        bool unfinished = false;
        std::uint64_t next = UINT64_MAX;
        for (auto handle : handles) {
            auto state = status(handle);
            unfinished |= state != Status::done;
            if (state == Status::waiting_time)
                next = std::min(next, integer(handle, "wake_time"));
        }
        if (!unfinished) return true;
        if (next == UINT64_MAX) return false; // event/channel deadlock
        advance(next);
    }
    return false;
}

void Process::advance(std::uint64_t time) {
    if (time < now_) throw std::invalid_argument("UVM time cannot move backwards");
    now_ = time;
    if (vir::runtime::abi::tick() != transport::Status::SUCCESS)
        throw std::runtime_error("UVM clock synchronization failed");
    std::vector<ObjectHandle> handles;
    { std::lock_guard lock(mutex_); handles = order_; }
    for (auto handle : handles)
        if (status(handle) == Status::waiting_time &&
            integer(handle, "wake_time") <= now_)
            set_status(handle, Status::ready);
    notify("clk");
}

void Process::notify(std::string_view event) {
    std::vector<ObjectHandle> handles;
    { std::lock_guard lock(mutex_); handles = order_; }
    for (auto handle : handles) {
        if (status(handle) != Status::waiting_event) continue;
        auto value = objects_.load(handle, "event");
        if (value.ready() && std::get<std::string>(value.value()) == event)
            set_status(handle, Status::ready);
    }
}

void Process::reset() {
    std::lock_guard lock(mutex_);
    for (auto handle : order_) objects_.destroy(handle);
    steps_.clear(); registered_.clear(); order_.clear(); now_ = 0;
}
std::size_t Process::size() const noexcept {
    std::lock_guard lock(mutex_);
    return order_.size();
}

void Process::set_pc(ObjectHandle process, std::uint64_t value) {
    if (!objects_.store(process, "pc", value).ready())
        throw std::runtime_error("invalid UVM process");
}
std::uint64_t Process::pc(ObjectHandle process) const { return integer(process, "pc"); }
void Process::store(ObjectHandle process, std::string_view name, Value value) {
    if (!objects_.store(process, local(name), std::move(value)).ready())
        throw std::runtime_error("invalid UVM process");
}
Value Process::load(ObjectHandle process, std::string_view name) const {
    auto result = objects_.load(process, local(name));
    if (!result.ready()) throw std::runtime_error("missing serialized process local " + std::string(name));
    return result.value();
}
void Process::wait_time(ObjectHandle process, std::uint64_t delay) {
    objects_.store(process, "wake_time", now_ + delay);
    set_status(process, Status::waiting_time);
}
void Process::wait_event(ObjectHandle process, std::string_view event) {
    objects_.store(process, "event", std::string(event));
    set_status(process, Status::waiting_event);
}
void Process::complete(ObjectHandle process) { set_status(process, Status::done); }
Process::Status Process::status(ObjectHandle process) const {
    return static_cast<Status>(integer(process, "status"));
}
ObjectHandle Process::owner(ObjectHandle process) const {
    auto result = objects_.load(process, "owner");
    if (!result.ready()) throw std::runtime_error("invalid UVM process owner");
    if (auto value = std::get_if<ObjectHandle>(&result.value())) return *value;
    throw std::runtime_error("invalid UVM process owner type");
}
ObjectHandle Process::normalize_owner(ObjectHandle value) const {
    auto type = objects_.type(value);
    return type.ready() && type.value() == process_type ? owner(value) : value;
}
void Process::set_status(ObjectHandle process, Status value) {
    if (!objects_.store(process, "status", std::uint64_t(value)).ready())
        throw std::runtime_error("invalid UVM process");
}
std::uint64_t Process::integer(ObjectHandle process, std::string_view field) const {
    auto result = objects_.load(process, field);
    if (!result.ready()) throw std::runtime_error("invalid UVM process field " + std::string(field));
    if (auto value = std::get_if<std::uint64_t>(&result.value())) return *value;
    if (auto value = std::get_if<std::int64_t>(&result.value())) return static_cast<std::uint64_t>(*value);
    throw std::runtime_error("non-integer UVM process field " + std::string(field));
}
} // namespace vir::runtime::builtin::uvm
