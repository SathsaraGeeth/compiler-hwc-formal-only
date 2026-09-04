#include "Builtin/UVM/UVM.h"
#include <atomic>
#include <stdexcept>

namespace {
std::atomic<std::uint64_t> objections{0};
}

namespace vir::runtime::builtin {
void UVM::reset_services() {
    factory_.reset();
    config_.reset();
    sequences_.reset();
    coverage_.reset();
    report_.reset();
    components_.reset();
    tlm_.reset();
    processes_.reset();
    collections_.reset();
    reset();
}
void UVM::raise_objection() noexcept { objections.fetch_add(1); }
void UVM::drop_objection() {
    auto count = objections.load();
    do {
        if (!count) throw std::runtime_error("UVM objection underflow");
    } while (!objections.compare_exchange_weak(count, count - 1));
}
std::uint64_t UVM::objection_count() noexcept { return objections.load(); }
void UVM::reset() noexcept { objections.store(0); }
} // namespace vir::runtime::builtin
