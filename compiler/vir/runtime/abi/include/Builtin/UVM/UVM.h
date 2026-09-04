#pragma once

#include <cstdint>
#include "Builtin/UVM/ConfigDB.h"
#include "Builtin/UVM/Collection.h"
#include "Builtin/UVM/Component.h"
#include "Builtin/UVM/Coverage.h"
#include "Builtin/UVM/Factory.h"
#include "Builtin/UVM/Report.h"
#include "Builtin/UVM/Process.h"
#include "Builtin/UVM/Sequencer.h"
#include "Builtin/UVM/TLM.h"

namespace vir::runtime::builtin {

class UVM {
public:
    explicit UVM(ObjectStore& objects)
        : factory_(objects), tlm_(objects), processes_(objects), collections_(objects) {}
    uvm::Factory& factory() noexcept { return factory_; }
    uvm::ConfigDB& config() noexcept { return config_; }
    uvm::Sequencer& sequences() noexcept { return sequences_; }
    uvm::Coverage& coverage() noexcept { return coverage_; }
    uvm::Report& report() noexcept { return report_; }
    uvm::Component& components() noexcept { return components_; }
    uvm::TLM& tlm() noexcept { return tlm_; }
    uvm::Process& processes() noexcept { return processes_; }
    uvm::Collection& collections() noexcept { return collections_; }
    void reset_services();
    static void raise_objection() noexcept;
    static void drop_objection();
    static std::uint64_t objection_count() noexcept;
    static void reset() noexcept;
private:
    uvm::Factory factory_;
    uvm::ConfigDB config_;
    uvm::Sequencer sequences_;
    uvm::Coverage coverage_;
    uvm::Report report_;
    uvm::Component components_;
    uvm::TLM tlm_;
    uvm::Process processes_;
    uvm::Collection collections_;
};

} // namespace vir::runtime::builtin
