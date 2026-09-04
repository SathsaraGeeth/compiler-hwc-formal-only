#include "Builtin/Assert/Assert.h"
#include "Builtin/Time/Time.h"
#include "Builtin/UVM/UVM.h"
#include "ABI/Runtime.h"
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <mutex>
#include <vector>

namespace {
vir::runtime::builtin::Time runtime_clock;
struct NBAUpdate { void* storage; std::uint64_t value; std::uint32_t bytes; };
std::mutex nba_mutex;
std::vector<NBAUpdate> nba_updates;
}

extern "C" void vir_assert_check(bool condition)
    asm("vir.runtime.assert.check");
extern "C" void vir_assert_check(bool condition) {
    vir::runtime::builtin::Assert::check(condition);
}

extern "C" void vir_time_delay(std::int32_t duration)
    asm("vir.runtime.time.delay");
extern "C" void vir_time_delay(std::int32_t duration) {
    if (duration > 0)
        runtime_clock.delay(static_cast<std::uint64_t>(duration));
    if (vir::runtime::abi::synchronize() != transport::Status::SUCCESS)
        throw std::runtime_error("VIR transport synchronization failed");
}

extern "C" void vir_finish()
    asm("vir.runtime.finish");
extern "C" void vir_finish() {}

extern "C" void vir_nba_store_u64(void* storage, std::uint64_t value,
                                    std::uint32_t bytes)
    asm("vir.runtime.nba.store.u64");
extern "C" void vir_nba_store_u64(void* storage, std::uint64_t value,
                                    std::uint32_t bytes) {
    if (!storage || !bytes || bytes > sizeof(value))
        throw std::invalid_argument("invalid NBA store");
    std::lock_guard lock(nba_mutex);
    nba_updates.push_back({storage, value, bytes});
}

namespace vir::runtime::abi {
void flush_nba() {
    std::vector<NBAUpdate> updates;
    { std::lock_guard lock(nba_mutex); updates.swap(nba_updates); }
    for (const auto& update : updates)
        std::memcpy(update.storage, &update.value, update.bytes);
}
}

extern "C" void vir_uvm_objection_raise()
    asm("vir.runtime.uvm.objection.raise");
extern "C" void vir_uvm_objection_raise() {
    vir::runtime::builtin::UVM::raise_objection();
}

extern "C" void vir_uvm_objection_drop()
    asm("vir.runtime.uvm.objection.drop");
extern "C" void vir_uvm_objection_drop() {
    vir::runtime::builtin::UVM::drop_objection();
}
