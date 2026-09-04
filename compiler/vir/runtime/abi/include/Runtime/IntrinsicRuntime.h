#pragma once

#include "External/Registry.h"
#include "Object/ObjectStore.h"
#include "Transport/EvaluateDut.h"
#include "Transport/SignalAccess.h"
#include "Builtin/UVM/UVM.h"
#include <atomic>
#include <memory>
#include <string>

namespace vir::runtime {

class IntrinsicRuntime {
public:
    explicit IntrinsicRuntime(transport::Transport& transport)
        : transport_(transport), signals_(transport), uvm_(objects_) {}

    std::unique_ptr<EvaluateDut> evaluate_dut(std::string instance) {
        return std::make_unique<EvaluateDut>(
            transport_, next_job_.fetch_add(1), std::move(instance));
    }

    SignalAccess& signals() noexcept { return signals_; }
    ExternalRegistry& externals() noexcept { return externals_; }
    ObjectStore& objects() noexcept { return objects_; }
    builtin::UVM& uvm() noexcept { return uvm_; }

private:
    transport::Transport& transport_;
    std::atomic<uint64_t> next_job_{1};
    SignalAccess signals_;
    ExternalRegistry externals_;
    ObjectStore objects_;
    builtin::UVM uvm_;
};

} // namespace vir::runtime
