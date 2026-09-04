#pragma once

#include "runtime/transport/transport.h"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace vir::runtime::abi::detail {

extern std::atomic<transport::Transport*> active_transport;
extern std::atomic<std::uint64_t> next_job;

struct SignalBinding {
    void* storage;
    transport::Signal signal;
    std::uint32_t width;
    bool input;
};

extern std::mutex design_mutex;
extern std::vector<SignalBinding> signal_bindings;
extern std::string instance_name;
extern void (*combinational_callback)();

std::int32_t status(transport::Status value) noexcept;
transport::Signal signal_id(const void* address);

} // namespace vir::runtime::abi::detail
