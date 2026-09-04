#include "Builtin/Assert/Assert.h"
#include "Builtin/Assume/Assume.h"
#include "Builtin/Channel/Channel.h"
#include "Builtin/Clock/Clock.h"
#include "Builtin/Constraint/Constraint.h"
#include "Builtin/Cover/Cover.h"
#include "Builtin/DUT/DUT.h"
#include "Builtin/Error/Error.h"
#include "Builtin/Event/Event.h"
#include "Builtin/Formal/Formal.h"
#include "Builtin/Logger/Logger.h"
#include "Builtin/Mutex/Mutex.h"
#include "Builtin/Object/Object.h"
#include "Builtin/Port/Port.h"
#include "Builtin/Process/Process.h"
#include "Builtin/Property/Property.h"
#include "Builtin/Random/Random.h"
#include "Builtin/Scheduler/Scheduler.h"
#include "Builtin/Semaphore/Semaphore.h"
#include "Builtin/Time/Time.h"
#include "Builtin/Transaction/Transaction.h"
#include "Builtin/Type/Type.h"
#include <atomic>

using namespace vir::runtime;
using namespace vir::runtime::builtin;

namespace {
class Transport final : public transport::Transport {
public:
    transport::Status try_schedule_job(transport::Job&) override { return transport::Status::SUCCESS; }
    transport::Status try_export_signal(transport::Signal, const transport::Value& value) override { stored = value; return transport::Status::SUCCESS; }
    transport::Status try_import_signal(transport::Signal, transport::Value& value, bool) override { value = stored; return transport::Status::SUCCESS; }
    transport::Value stored{0, 0, 0, 1};
};
}

int main() {
    Time time;
    if (time.now() > 1000000000) return 1;
    Event event; event.trigger(); if (!event.is_triggered()) return 2;
    Mutex mutex; if (!mutex.try_lock()) return 3; mutex.unlock();
    Semaphore semaphore(1); if (!semaphore.try_acquire()) return 4; semaphore.release();
    Channel channel; channel.send(uint64_t{3}); if (std::get<uint64_t>(channel.receive()) != 3) return 5;
    std::atomic<bool> ran = false;
    Process process([&](std::stop_token) { ran = true; }); process.spawn(); process.join();
    if (!ran || process.state() != Process::State::completed) return 6;
    Scheduler scheduler; scheduler.schedule([&] { ran = false; }, 7); scheduler.run();
    if (ran || scheduler.now() != 7) return 7;
    if (Type::name(Value{uint64_t{1}}) != "u64") return 8;

    ObjectStore store;
    ClassDescriptor descriptor;
    descriptor.name = "native";
    descriptor.construct = [](auto) { return Result<ClassDescriptor::NativeObject>::completed(std::make_shared<uint64_t>(9)); };
    descriptor.methods["get"] = [](const auto& native, auto) { return Result<Value>::completed(*native_cast<uint64_t>(native)); };
    if (!store.add_class(std::move(descriptor))) return 9;
    auto native = store.create("native", {}); if (!native.ready()) return 10;
    auto native_value = store.call(native.value(), "get", {});
    if (!native_value.ready() || std::get<uint64_t>(native_value.value()) != 9) return 11;
    Object object(store); auto clone = object.clone(native.value());
    if (!clone.ready() || !object.compare(native.value(), clone.value()).value()) return 12;

    try { Assert::check(false); return 13; } catch (const AssertionFailure&) {}
    Assume assume; assume.add(true); if (!assume.valid()) return 14;
    Cover cover; cover.hit("bin"); if (cover.hits("bin") != 1) return 15;
    Property property([] { return true; }); if (!property.evaluate()) return 16;
    Random random(1); if (random.range(4, 4) != 4) return 17;
    Constraint constraint; constraint.add([] { return true; }); if (!constraint.solve()) return 18;
    bool logged = false; Logger logger([&](auto, auto) { logged = true; }); logger.info("ok"); if (!logged) return 19;
    bool caught = false; Error::catch_error([] { Error::raise("x"); }, [&](auto&) { caught = true; }); if (!caught) return 20;

    Transport transport;
    DUT dut(transport, "@dut"); if (dut.evaluate()->execute() != transport::Status::SUCCESS) return 21;
    Transaction transaction("packet"); transaction.set("data", uint64_t{5});
    if (std::get<uint64_t>(transaction.get("data").value()) != 5 || !transaction.compare(transaction.clone())) return 22;
    Port source, sink; source.connect(sink); source.send(uint64_t{6}); if (std::get<uint64_t>(sink.receive()) != 6) return 23;
    Formal formal; formal.assert_property("p", property); if (!formal.prove()) return 24;
    Clock clock(1000000); clock.start(); time.delay(3000000); clock.stop();
    return 0;
}
