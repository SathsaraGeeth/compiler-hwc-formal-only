#include "../ABI/Context.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {

using transport::Status;
using vir::runtime::abi::detail::active_transport;
using vir::runtime::abi::detail::signal_id;
using vir::runtime::abi::detail::status;

struct ScheduleResult {
    std::int32_t status;
    void* job;
};

template<class T>
std::int32_t export_signal(void* signal, T value, std::uint32_t width) {
    auto* transport = active_transport.load(std::memory_order_acquire);
    if (!transport) return status(Status::ERROR);
    auto result = Status::RETRY;
    transport::Value outgoing{static_cast<std::uint64_t>(value), 0, 0, width};
    while (result == Status::RETRY) {
        result = transport->try_export_signal(signal_id(signal), outgoing);
        if (result == Status::RETRY) std::this_thread::yield();
    }
    if (std::getenv("HWC_TRACE_TRANSPORT"))
        std::cerr << "VIR export signal=" << signal_id(signal)
                  << " width=" << width << " value=" << outgoing.data << '\n';
    return status(result);
}

std::uint64_t import_signal(void* signal, bool peek, std::uint32_t width) {
    auto* transport = active_transport.load(std::memory_order_acquire);
    if (!transport) return static_cast<std::uint32_t>(status(Status::ERROR));
    transport::Value value{0, 0, 0, width};
    auto result = Status::RETRY;
    while (result == Status::RETRY) {
        result = transport->try_import_signal(signal_id(signal), value, peek);
        if (result == Status::RETRY) std::this_thread::yield();
    }
    if (result != Status::SUCCESS || value.width != width)
        return static_cast<std::uint32_t>(status(
            result == Status::SUCCESS ? Status::ERROR : result));
    if (std::getenv("HWC_TRACE_TRANSPORT"))
        std::cerr << "VIR import signal=" << signal_id(signal)
                  << " width=" << width << " value=" << value.data
                  << " xmask=" << value.xmask << '\n';
    return (value.zmask << 24) | (value.xmask << 16) |
           (value.data << 8) |
           static_cast<std::uint8_t>(status(result));
}

} // namespace

extern "C" std::int32_t vir_export_i1(void* signal, bool value)
    asm("vir.runtime.transport.export.i1");
extern "C" std::int32_t vir_export_i1(void* signal, bool value) {
    return export_signal(signal, value, 1);
}

extern "C" std::int32_t vir_export_i8(void* signal, std::uint8_t value)
    asm("vir.runtime.transport.export.i8");
extern "C" std::int32_t vir_export_i8(void* signal, std::uint8_t value) {
    return export_signal(signal, value, 8);
}

extern "C" std::uint64_t vir_import_i1(void* signal, bool peek)
    asm("vir.runtime.transport.import.i1");
extern "C" std::uint64_t vir_import_i1(void* signal, bool peek) {
    return import_signal(signal, peek, 1);
}

extern "C" std::uint64_t vir_import_i8(void* signal, bool peek)
    asm("vir.runtime.transport.import.i8");
extern "C" std::uint64_t vir_import_i8(void* signal, bool peek) {
    return import_signal(signal, peek, 8);
}

extern "C" ScheduleResult vir_schedule(const char* instance)
    asm("vir.runtime.transport.schedule");
extern "C" ScheduleResult vir_schedule(const char* instance) {
    auto* transport = active_transport.load(std::memory_order_acquire);
    if (!transport) return {status(Status::ERROR), nullptr};
    transport::Job job{
        vir::runtime::abi::detail::next_job.fetch_add(1), instance};
    auto result = Status::RETRY;
    while (result == Status::RETRY) {
        result = transport->try_schedule_job(job);
        if (result == Status::RETRY) std::this_thread::yield();
    }
    auto* handle = result == Status::SUCCESS
        ? const_cast<char*>(instance) : nullptr;
    return {status(result), handle};
}
