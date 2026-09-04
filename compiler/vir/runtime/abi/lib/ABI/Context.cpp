#include "ABI/Runtime.h"
#include "Context.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace vir::runtime::abi::detail {

std::atomic<transport::Transport*> active_transport;
std::atomic<std::uint64_t> next_job{1};
std::mutex design_mutex;
std::vector<SignalBinding> signal_bindings;
std::vector<void*> clock_bindings;
std::string instance_name;
void (*combinational_callback)() = nullptr;
void (*clocked_callback)() = nullptr;

std::int32_t status(transport::Status value) noexcept {
    return static_cast<std::int32_t>(value);
}

transport::Signal signal_id(const void* address) {
    std::lock_guard lock(design_mutex);
    for (const auto& binding : signal_bindings)
        if (binding.storage == address) return binding.signal;
    return 0;
}

} // namespace vir::runtime::abi::detail

namespace vir::runtime::abi {

void install_transport(transport::Transport& transport) noexcept {
    detail::active_transport.store(&transport, std::memory_order_release);
}

void remove_transport() noexcept {
    detail::active_transport.store(nullptr, std::memory_order_release);
}

void clear_design() noexcept {
    std::lock_guard lock(detail::design_mutex);
    detail::signal_bindings.clear();
    detail::clock_bindings.clear();
    detail::instance_name.clear();
    detail::combinational_callback = nullptr;
    detail::clocked_callback = nullptr;
}

void set_combinational(void (*callback)()) noexcept {
    std::lock_guard lock(detail::design_mutex);
    detail::combinational_callback = callback;
}
void set_clocked(void (*callback)()) noexcept {
    std::lock_guard lock(detail::design_mutex);
    detail::clocked_callback = callback;
}

bool register_signal(void* storage, transport::Signal signal,
                     std::uint32_t width, bool input) {
    if (!storage || !width || width > 64) return false;
    std::lock_guard lock(detail::design_mutex);
    detail::signal_bindings.push_back({storage, signal, width, input});
    return true;
}
bool register_clock(void* storage) {
    if (!storage) return false;
    std::lock_guard lock(detail::design_mutex);
    detail::clock_bindings.push_back(storage);
    return true;
}

transport::Status tick() {
    using transport::Status;
    if (!detail::active_transport.load(std::memory_order_acquire))
        return Status::SUCCESS;
    std::vector<void*> clocks;
    { std::lock_guard lock(detail::design_mutex); clocks = detail::clock_bindings; }
    for (auto* clock : clocks) *static_cast<std::uint8_t*>(clock) = 0;
    if (auto result = synchronize(); result != Status::SUCCESS) return result;
    for (auto* clock : clocks) *static_cast<std::uint8_t*>(clock) = 1;
    auto result = synchronize();
    if (result != Status::SUCCESS) return result;
    void (*clocked)() = nullptr;
    { std::lock_guard lock(detail::design_mutex); clocked = detail::clocked_callback; }
    if (clocked) {
        clocked();
        flush_nba();
    }
    return Status::SUCCESS;
}

void set_instance(std::string instance) {
    std::lock_guard lock(detail::design_mutex);
    detail::instance_name = std::move(instance);
}

transport::Status synchronize() {
    using transport::Status;
    auto* active = detail::active_transport.load(std::memory_order_acquire);
    if (!active) return Status::ERROR;
    std::lock_guard lock(detail::design_mutex);
    if (detail::combinational_callback) detail::combinational_callback();
    for (const auto& binding : detail::signal_bindings) {
        if (!binding.input) continue;
        std::uint64_t data = 0;
        std::memcpy(&data, binding.storage, (binding.width + 7) / 8);
        transport::Value value{data, 0, 0, binding.width};
        auto result = Status::RETRY;
        while (result == Status::RETRY) {
            result = active->try_export_signal(binding.signal, value);
            if (result == Status::RETRY) std::this_thread::yield();
        }
        if (result != Status::SUCCESS) return result;
        if (std::getenv("HWC_TRACE_TRANSPORT"))
            std::cerr << "VIR export signal=" << binding.signal
                      << " width=" << binding.width
                      << " value=" << value.data << '\n';
    }
    transport::Job job{detail::next_job.fetch_add(1), detail::instance_name.c_str()};
    auto scheduled = Status::RETRY;
    while (scheduled == Status::RETRY) {
        scheduled = active->try_schedule_job(job);
        if (scheduled == Status::RETRY) std::this_thread::yield();
    }
    if (scheduled != Status::SUCCESS) return scheduled;
    for (const auto& binding : detail::signal_bindings) {
        if (binding.input) continue;
        transport::Value value{0, 0, 0, binding.width};
        auto result = Status::RETRY;
        while (result == Status::RETRY) {
            result = active->try_import_signal(binding.signal, value, false);
            if (result == Status::RETRY) std::this_thread::yield();
        }
        if (result != Status::SUCCESS) return result;
        if (std::getenv("HWC_TRACE_TRANSPORT"))
            std::cerr << "VIR import signal=" << binding.signal
                      << " width=" << binding.width
                      << " value=" << value.data
                      << " xmask=" << value.xmask << '\n';
        std::memcpy(binding.storage, &value.data, (binding.width + 7) / 8);
    }
    return Status::SUCCESS;
}

} // namespace vir::runtime::abi
