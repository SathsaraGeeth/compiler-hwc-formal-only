#include "ABI/Runtime.h"
#include <cassert>
#include <cstdint>
#include <string>

namespace {

struct ScheduleResult {
    std::int32_t status;
    void* job;
};

class TestTransport final : public transport::Transport {
public:
    transport::Status try_schedule_job(transport::Job& job) override {
        instance = job.instance;
        return transport::Status::SUCCESS;
    }

    transport::Status try_export_signal(
        transport::Signal id, const transport::Value& next) override {
        signal = id;
        value = next;
        return transport::Status::SUCCESS;
    }

    transport::Status try_import_signal(
        transport::Signal id, transport::Value& result, bool) override {
        if (id != signal) return transport::Status::ERROR;
        result = value;
        return transport::Status::SUCCESS;
    }

    transport::Signal signal{};
    transport::Value value{};
    std::string instance;
};

} // namespace

extern "C" void runtime_assert(bool) asm("vir.runtime.assert.check");
extern "C" std::int32_t runtime_export(void*, std::uint8_t)
    asm("vir.runtime.transport.export.i8");
extern "C" ScheduleResult runtime_schedule(const char*)
    asm("vir.runtime.transport.schedule");

int main() {
    TestTransport transport;
    vir::runtime::abi::install_transport(transport);

    std::uint8_t signal = 0;
    assert(runtime_export(&signal, 42) == 0);
    assert(transport.value.data == 42);
    assert(transport.value.width == 8);

    auto scheduled = runtime_schedule("dut");
    assert(scheduled.status == 0);
    assert(transport.instance == "dut");
    runtime_assert(true);

    vir::runtime::abi::remove_transport();
}
