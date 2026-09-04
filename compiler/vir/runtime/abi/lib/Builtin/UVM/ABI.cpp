#include "ABI/UVM.h"
#include "Builtin/UVM/UVM.h"
#include "Object/ObjectStore.h"
#include "Builtin/Random/Random.h"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

namespace {
vir::runtime::ObjectStore objects;
vir::runtime::builtin::UVM uvm(objects);
vir::runtime::builtin::Random uvm_random(1);

const char* required(const char* value, const char* argument) {
    if (!value) throw std::invalid_argument(std::string("null UVM ") + argument);
    return value;
}
vir::runtime::ObjectHandle handle(void* value) {
    return {static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(value))};
}
void* pointer(vir::runtime::ObjectHandle value) {
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(value.value));
}
}

extern "C" std::uint64_t vir_uvm_factory_create(
    const char* type, const char* name, std::uint64_t parent) {
    auto result = uvm.factory().create(required(type, "type"),
                                       required(name, "name"), {parent});
    if (!result.ready()) throw std::runtime_error(result.error());
    uvm.components().add(result.value(), type, name, {parent});
    return result.value().value;
}

extern "C" void vir_uvm_factory_override(
    const char* requested, const char* replacement) {
    uvm.factory().set_override(required(requested, "requested type"),
                               required(replacement, "replacement type"));
}

extern "C" void* vir_uvm_factory_create_ptr(
    const char* type, const char* name, void* parent) {
    auto handle = vir_uvm_factory_create(
        type, name, static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(parent)));
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(handle));
}
extern "C" bool vir_uvm_object_is_type(void* object, const char* type) {
    auto result = objects.type(handle(object));
    if (!result.ready()) return false;
    auto matches = result.value() == required(type, "object type");
    if (std::getenv("HWC_TRACE_UVM"))
        std::cerr << "UVM_CAST actual=" << result.value() << " target=" << type
                  << " matches=" << matches << '\n';
    return matches;
}
extern "C" std::uint64_t vir_uvm_object_load_u64(
    void* object, const char* field) {
    auto result = objects.load(handle(object), required(field, "object field"));
    if (!result.ready()) return 0;
    if (auto value = std::get_if<std::uint64_t>(&result.value())) return *value;
    if (auto value = std::get_if<std::int64_t>(&result.value())) return *value;
    return 0;
}
extern "C" void* vir_uvm_object_load_ptr(void* object, const char* field) {
    auto result = objects.load(handle(object), required(field, "object field"));
    if (!result.ready()) return nullptr;
    if (auto value = std::get_if<vir::runtime::ObjectHandle>(&result.value()))
        return pointer(*value);
    return nullptr;
}
extern "C" void vir_uvm_object_store_u64(
    void* object, const char* field, std::uint64_t value) {
    if (!objects.store(handle(object), required(field, "object field"), value).ready())
        throw std::runtime_error("invalid UVM object field store");
}
extern "C" void vir_uvm_object_store_ptr(
    void* object, const char* field, void* value) {
    if (!objects.store(handle(object), required(field, "object field"),
                       handle(value)).ready())
        throw std::runtime_error("invalid UVM object field store");
}
extern "C" bool vir_uvm_object_randomize(void* object) {
    auto selected = handle(object);
    if (!objects.type(selected).ready()) return false;
    objects.store(selected, "data", static_cast<std::uint64_t>(uvm_random.range(0, 255)));
    return true;
}
extern "C" std::int64_t vir_uvm_random_range(
    std::int64_t minimum, std::int64_t maximum) {
    return uvm_random.range(minimum, maximum);
}

extern "C" void vir_uvm_config_set_u64(
    const char* scope, const char* field, std::uint64_t value) {
    uvm.config().set(required(scope, "scope"), required(field, "field"), value);
}

extern "C" bool vir_uvm_config_get_u64(
    const char* instance, const char* field, std::uint64_t* value) {
    if (!value) return false;
    auto result = uvm.config().get(required(instance, "instance"),
                                   required(field, "field"));
    if (!result.ready()) return false;
    if (auto found = std::get_if<std::uint64_t>(&result.value())) {
        *value = *found;
        return true;
    }
    if (auto found = std::get_if<vir::runtime::ObjectHandle>(&result.value())) {
        *value = found->value;
        return true;
    }
    return false;
}

extern "C" void vir_uvm_config_set_ptr(
    void*, const char* scope, const char* field, void* value) {
    uvm.config().set(required(scope, "scope"), required(field, "field"),
                     vir::runtime::ObjectHandle{
                         static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(value))});
}

extern "C" void vir_uvm_sequence_start_u64(
    const char* sequencer, std::uint64_t item) {
    uvm.sequences().start_item(required(sequencer, "sequencer"), item);
}

extern "C" bool vir_uvm_sequence_try_next_u64(
    const char* sequencer, std::uint64_t* item) {
    if (!item) return false;
    auto result = uvm.sequences().try_next_item(required(sequencer, "sequencer"));
    if (!result) return false;
    if (auto found = std::get_if<std::uint64_t>(&*result)) {
        *item = *found;
        return true;
    }
    return false;
}

extern "C" void vir_uvm_sequence_start_ptr(void* sequence, void* sequencer) {
    auto process = uvm.processes().spawn("body", handle(sequence));
    uvm.processes().store(process, "sequencer", handle(sequencer));
}
extern "C" void vir_uvm_sequence_start_item_ptr(void* process, void* item) {
    auto sequencer = uvm.processes().load(handle(process), "sequencer");
    auto selected = std::get<vir::runtime::ObjectHandle>(sequencer);
    (void)selected;
    if (std::getenv("HWC_TRACE_UVM")) {
        auto type = objects.type(handle(item));
        std::cerr << "UVM_SEQUENCE_ITEM type="
                  << (type.ready() ? type.value() : "<invalid>");
        auto data = objects.load(handle(item), "data");
        if (data.ready() && std::holds_alternative<std::uint64_t>(data.value()))
            std::cerr << " data=" << std::get<std::uint64_t>(data.value());
        std::cerr << '\n';
    }
    uvm.sequences().start_item("sequencer.default", handle(item));
    uvm.processes().notify("sequencer.default");
}
extern "C" void vir_uvm_sequence_finish_item(void*) {
    // Completion is acknowledged by the driver's item_done operation. The
    // cooperative producer yields at its next explicit suspension point.
}
extern "C" void vir_uvm_sequence_item_done(void*) {}
extern "C" void* vir_uvm_sequence_try_get_ptr() {
    auto value = uvm.sequences().try_next_item("sequencer.default");
    if (!value) return nullptr;
    if (auto item = std::get_if<vir::runtime::ObjectHandle>(&*value))
        return pointer(*item);
    throw std::runtime_error("sequencer item is not an object");
}

extern "C" void* vir_uvm_process_create(void* owner, vir_uvm_process_step step) {
    if (!step) throw std::invalid_argument("null UVM process step");
    return pointer(uvm.processes().create(handle(owner),
        [step](vir::runtime::ObjectHandle process) { step(pointer(process)); }));
}
extern "C" void vir_uvm_process_register(
    const char* name, vir_uvm_process_step step) {
    if (!step) throw std::invalid_argument("null UVM process step");
    uvm.processes().register_step(required(name, "process name"),
        [step](vir::runtime::ObjectHandle process) { step(pointer(process)); });
}
extern "C" void* vir_uvm_process_spawn(const char* name, void* owner) {
    return pointer(uvm.processes().spawn(required(name, "process name"), handle(owner)));
}
extern "C" void vir_uvm_process_run() { uvm.processes().run(); }
extern "C" bool vir_uvm_process_drain(std::uint64_t max_steps) {
    return uvm.processes().drain(max_steps);
}
extern "C" void vir_uvm_process_advance(std::uint64_t time) {
    uvm.processes().advance(time);
}
extern "C" void vir_uvm_process_notify(const char* event) {
    uvm.processes().notify(required(event, "process event"));
}
extern "C" std::uint64_t vir_uvm_process_pc(void* process) {
    return uvm.processes().pc(handle(process));
}
extern "C" void vir_uvm_process_set_pc(void* process, std::uint64_t pc) {
    uvm.processes().set_pc(handle(process), pc);
}
extern "C" void vir_uvm_process_store_u64(
    void* process, const char* name, std::uint64_t value) {
    uvm.processes().store(handle(process), required(name, "local name"), value);
}
extern "C" std::uint64_t vir_uvm_process_load_u64(
    void* process, const char* name) {
    auto value = uvm.processes().load(handle(process), required(name, "local name"));
    if (auto result = std::get_if<std::uint64_t>(&value)) return *result;
    if (auto result = std::get_if<std::int64_t>(&value)) return *result;
    throw std::runtime_error("serialized UVM local is not an integer");
}
extern "C" void vir_uvm_process_store_ptr(
    void* process, const char* name, void* value) {
    uvm.processes().store(handle(process), required(name, "local name"), handle(value));
}
extern "C" void* vir_uvm_process_load_ptr(void* process, const char* name) {
    auto value = uvm.processes().load(handle(process), required(name, "local name"));
    if (auto result = std::get_if<vir::runtime::ObjectHandle>(&value)) return pointer(*result);
    throw std::runtime_error("serialized UVM local is not an object");
}
extern "C" void vir_uvm_process_wait_time(void* process, std::uint64_t delay) {
    uvm.processes().wait_time(handle(process), delay);
}
extern "C" void vir_uvm_process_wait_event(void* process, const char* event) {
    uvm.processes().wait_event(handle(process), required(event, "process event"));
}
extern "C" void vir_uvm_process_complete(void* process) {
    uvm.processes().complete(handle(process));
}
extern "C" void vir_uvm_collection_push_ptr(
    void* process, const char* name, void* value) {
    auto owner = uvm.processes().normalize_owner(handle(process));
    uvm.collections().push(owner, required(name, "collection name"), handle(value));
}
extern "C" void* vir_uvm_collection_get_ptr(
    void* process, const char* name, std::uint64_t index) {
    auto owner = uvm.processes().normalize_owner(handle(process));
    auto value = uvm.collections().get(owner, required(name, "collection name"), index);
    if (auto result = std::get_if<vir::runtime::ObjectHandle>(&value)) return pointer(*result);
    throw std::runtime_error("serialized UVM collection element is not an object");
}
extern "C" void vir_uvm_collection_erase(
    void* process, const char* name, std::uint64_t index) {
    auto owner = uvm.processes().normalize_owner(handle(process));
    uvm.collections().erase(owner, required(name, "collection name"), index);
}
extern "C" std::uint64_t vir_uvm_collection_size(void* process, const char* name) {
    auto owner = uvm.processes().normalize_owner(handle(process));
    return uvm.collections().size(owner, required(name, "collection name"));
}

extern "C" void vir_uvm_component_register(
    const char* type, const char* phase, vir_uvm_phase_callback callback) {
    if (!callback) throw std::invalid_argument("null UVM phase callback");
    uvm.components().register_type(
        required(type, "component type"), required(phase, "phase"),
        [callback](vir::runtime::ObjectHandle component) {
            callback(pointer(component));
        });
}

extern "C" void vir_uvm_component_run(const char* phase) {
    uvm.components().run(required(phase, "phase"));
}

extern "C" void vir_uvm_component_register_process(
    const char* type, vir_uvm_process_step step) {
    if (!step) throw std::invalid_argument("null UVM process step");
    uvm.components().register_type(required(type, "component type"), "run_phase",
        [step](vir::runtime::ObjectHandle component) {
            uvm.processes().create(component,
                [step](vir::runtime::ObjectHandle process) {
                    step(pointer(process));
                });
        });
}

extern "C" void vir_uvm_run_test(const char* type) {
    auto test = uvm.factory().create(required(type, "test type"), "uvm_test_top");
    if (!test.ready()) throw std::runtime_error(test.error());
    uvm.components().add(test.value(), type, "uvm_test_top");
    for (const auto* phase : {"build_phase", "connect_phase", "run_phase"})
        uvm.components().run(phase);
    if (uvm.processes().size() && !uvm.processes().drain())
        throw std::runtime_error("UVM run phase deadlocked or exceeded its step limit");
    uvm.components().run("check_phase");
}

extern "C" void vir_uvm_tlm_connect(const char* port, const char* endpoint) {
    uvm.tlm().connect(required(port, "TLM port"),
                      required(endpoint, "TLM endpoint"));
}

extern "C" void vir_uvm_tlm_write_ptr(const char* port, void* transaction) {
    if (std::getenv("HWC_TRACE_UVM")) {
        auto type = objects.type(handle(transaction));
        std::cerr << "UVM_TLM_WRITE port=" << required(port, "TLM port")
                  << " type=" << (type.ready() ? type.value() : "<invalid>") << '\n';
    }
    auto fifos = uvm.tlm().write(required(port, "TLM port"), handle(transaction));
    for (const auto& fifo : fifos) uvm.processes().notify("tlm." + fifo);
}

extern "C" void vir_uvm_tlm_fifo_push_ptr(const char* fifo, void* transaction) {
    auto name = required(fifo, "TLM FIFO");
    uvm.tlm().fifo_push(name, handle(transaction));
    uvm.processes().notify(std::string("tlm.") + name);
}
extern "C" void vir_uvm_tlm_subscribe(
    const char* endpoint, void (*callback)(void*)) {
    if (!callback) throw std::invalid_argument("null UVM TLM subscriber");
    uvm.tlm().subscribe(required(endpoint, "TLM subscriber endpoint"),
        [callback](const vir::runtime::Value& value) {
            auto object = std::get_if<vir::runtime::ObjectHandle>(&value);
            if (!object) throw std::runtime_error("TLM subscriber value is not an object");
            callback(pointer(*object));
        });
}
extern "C" void* vir_uvm_tlm_fifo_try_get_ptr(const char* fifo) {
    auto value = uvm.tlm().fifo_try_get(required(fifo, "TLM FIFO"));
    if (!value) return nullptr;
    if (auto item = std::get_if<vir::runtime::ObjectHandle>(&*value)) return pointer(*item);
    throw std::runtime_error("TLM FIFO item is not an object");
}

extern "C" void vir_uvm_coverage_register(const char* bin) {
    uvm.coverage().register_bin(required(bin, "coverage bin"));
}

extern "C" void vir_uvm_coverage_sample_u64(
    const char* bin, std::uint64_t value) {
    uvm.coverage().sample(required(bin, "coverage bin"), value);
}
extern "C" void vir_uvm_coverage_sample_range_u64(
    const char* bin, std::uint64_t value, std::uint64_t low,
    std::uint64_t high) {
    if (low <= value && value <= high) {
        uvm.coverage().sample(required(bin, "coverage bin"), value);
        if (std::getenv("HWC_TRACE_UVM"))
            std::cerr << "UVM_COVERAGE_SAMPLE bin=" << bin
                      << " value=" << value << '\n';
    }
}

extern "C" double vir_uvm_coverage_percentage() {
    return uvm.coverage().percentage();
}

extern "C" void vir_uvm_coverage_report() {
    uvm.coverage().report(std::cout);
}

extern "C" void vir_uvm_report(
    std::uint32_t severity, const char* id, const char* message,
    std::int32_t verbosity, const char* file, std::int32_t line) {
    if (severity > static_cast<std::uint32_t>(
                       vir::runtime::builtin::uvm::Severity::fatal))
        throw std::invalid_argument("invalid UVM report severity");
    uvm.report().emit(
        static_cast<vir::runtime::builtin::uvm::Severity>(severity),
        required(id, "report id"), required(message, "report message"),
        verbosity, file ? file : "", line);
}

extern "C" void vir_uvm_report_summary() { uvm.report().summary(std::cout); }

extern "C" std::uint64_t vir_uvm_error_count() {
    return uvm.report().errors() + uvm.report().fatals();
}

extern "C" void vir_uvm_reset() { uvm.reset_services(); }
