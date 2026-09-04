#pragma once
#include "../../Transport/EvaluateDut.h"
#include "../../Transport/SignalAccess.h"
#include <atomic>
#include <memory>
#include <string>

namespace vir::runtime::builtin {
class DUT {
public:
    DUT(transport::Transport& transport, std::string instance)
        : transport_(transport), instance_(std::move(instance)), signals_(transport) {}
    std::unique_ptr<EvaluateDut> evaluate();
    Result<void> write(transport::Signal signal, const transport::Value& value);
    Result<transport::Value> read(transport::Signal signal, bool peek = false);
    Result<void> force(transport::Signal signal, const transport::Value& value);
    Result<void> release(transport::Signal signal);
private:
    transport::Transport& transport_;
    std::string instance_;
    SignalAccess signals_;
    std::atomic<uint64_t> next_job_{1};
};
}
