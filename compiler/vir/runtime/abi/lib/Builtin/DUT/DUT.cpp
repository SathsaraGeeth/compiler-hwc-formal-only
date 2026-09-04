#include "Builtin/DUT/DUT.h"

namespace vir::runtime::builtin {
std::unique_ptr<EvaluateDut> DUT::evaluate() {
    return std::make_unique<EvaluateDut>(transport_, next_job_.fetch_add(1), instance_);
}
Result<void> DUT::write(transport::Signal signal, const transport::Value& value) {
    const auto status = signals_.export_signal(signal, value);
    if (status == transport::Status::SUCCESS) return Result<void>::completed();
    if (status == transport::Status::RETRY) return Result<void>::retry();
    return Result<void>::failed("DUT write failed");
}
Result<transport::Value> DUT::read(transport::Signal signal, bool peek) {
    transport::Value value{};
    const auto status = signals_.import_signal(signal, value, peek);
    if (status == transport::Status::SUCCESS) return Result<transport::Value>::completed(value);
    if (status == transport::Status::RETRY) return Result<transport::Value>::retry();
    return Result<transport::Value>::failed("DUT read failed");
}
Result<void> DUT::force(transport::Signal signal, const transport::Value& value) { return write(signal, value); }
Result<void> DUT::release(transport::Signal signal) { return write(signal, transport::Value{0, 0, 1, 1}); }
}
