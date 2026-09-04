#pragma once

#include <cstdint>

extern "C" {

std::uint64_t vir_uvm_factory_create(const char* type, const char* name,
                                     std::uint64_t parent)
    asm("vir.runtime.uvm.factory.create");
void vir_uvm_factory_override(const char* requested, const char* replacement)
    asm("vir.runtime.uvm.factory.override");
void* vir_uvm_factory_create_ptr(const char* type, const char* name, void* parent)
    asm("vir.runtime.uvm.factory.create.ptr");
bool vir_uvm_object_is_type(void* object, const char* type)
    asm("vir.runtime.uvm.object.is_type");
std::uint64_t vir_uvm_object_load_u64(void* object, const char* field)
    asm("vir.runtime.uvm.object.load.u64");
void* vir_uvm_object_load_ptr(void* object, const char* field)
    asm("vir.runtime.uvm.object.load.ptr");
void vir_uvm_object_store_u64(void* object, const char* field,
                              std::uint64_t value)
    asm("vir.runtime.uvm.object.store.u64");
void vir_uvm_object_store_ptr(void* object, const char* field, void* value)
    asm("vir.runtime.uvm.object.store.ptr");
bool vir_uvm_object_randomize(void* object)
    asm("vir.runtime.uvm.object.randomize");
std::int64_t vir_uvm_random_range(std::int64_t minimum, std::int64_t maximum)
    asm("vir.runtime.uvm.random.range");

void vir_uvm_config_set_u64(const char* scope, const char* field,
                            std::uint64_t value)
    asm("vir.runtime.uvm.config.set.u64");
bool vir_uvm_config_get_u64(const char* instance, const char* field,
                            std::uint64_t* value)
    asm("vir.runtime.uvm.config.get.u64");
void vir_uvm_config_set_ptr(void* context, const char* scope,
                            const char* field, void* value)
    asm("vir.runtime.uvm.config.set.ptr");

void vir_uvm_sequence_start_u64(const char* sequencer, std::uint64_t item)
    asm("vir.runtime.uvm.sequence.start.u64");
bool vir_uvm_sequence_try_next_u64(const char* sequencer, std::uint64_t* item)
    asm("vir.runtime.uvm.sequence.try_next.u64");
void vir_uvm_sequence_start_ptr(void* sequence, void* sequencer)
    asm("vir.runtime.uvm.sequence.start.ptr");
void vir_uvm_sequence_start_item_ptr(void* process, void* item)
    asm("vir.runtime.uvm.sequence.start_item.ptr");
void vir_uvm_sequence_finish_item(void* process)
    asm("vir.runtime.uvm.sequence.finish_item");
void vir_uvm_sequence_item_done(void* process)
    asm("vir.runtime.uvm.sequence.item_done");
void* vir_uvm_sequence_try_get_ptr()
    asm("vir.runtime.uvm.sequence.try_get.ptr");

using vir_uvm_process_step = void (*)(void* process);
void vir_uvm_process_register(const char* name, vir_uvm_process_step step)
    asm("vir.runtime.uvm.process.register");
void* vir_uvm_process_spawn(const char* name, void* owner)
    asm("vir.runtime.uvm.process.spawn");
void* vir_uvm_process_create(void* owner, vir_uvm_process_step step)
    asm("vir.runtime.uvm.process.create");
void vir_uvm_process_run() asm("vir.runtime.uvm.process.run");
bool vir_uvm_process_drain(std::uint64_t max_steps)
    asm("vir.runtime.uvm.process.drain");
void vir_uvm_process_advance(std::uint64_t time)
    asm("vir.runtime.uvm.process.advance");
void vir_uvm_process_notify(const char* event)
    asm("vir.runtime.uvm.process.notify");
std::uint64_t vir_uvm_process_pc(void* process)
    asm("vir.runtime.uvm.process.pc");
void vir_uvm_process_set_pc(void* process, std::uint64_t pc)
    asm("vir.runtime.uvm.process.set_pc");
void vir_uvm_process_store_u64(void* process, const char* name,
                               std::uint64_t value)
    asm("vir.runtime.uvm.process.store.u64");
std::uint64_t vir_uvm_process_load_u64(void* process, const char* name)
    asm("vir.runtime.uvm.process.load.u64");
void vir_uvm_process_store_ptr(void* process, const char* name, void* value)
    asm("vir.runtime.uvm.process.store.ptr");
void* vir_uvm_process_load_ptr(void* process, const char* name)
    asm("vir.runtime.uvm.process.load.ptr");
void vir_uvm_process_wait_time(void* process, std::uint64_t delay)
    asm("vir.runtime.uvm.process.wait_time");
void vir_uvm_process_wait_event(void* process, const char* event)
    asm("vir.runtime.uvm.process.wait_event");
void vir_uvm_process_complete(void* process)
    asm("vir.runtime.uvm.process.complete");
void vir_uvm_collection_push_ptr(void* process, const char* name, void* value)
    asm("vir.runtime.uvm.collection.push.ptr");
void* vir_uvm_collection_get_ptr(void* process, const char* name,
                                 std::uint64_t index)
    asm("vir.runtime.uvm.collection.get.ptr");
void vir_uvm_collection_erase(void* process, const char* name,
                              std::uint64_t index)
    asm("vir.runtime.uvm.collection.erase");
std::uint64_t vir_uvm_collection_size(void* process, const char* name)
    asm("vir.runtime.uvm.collection.size");

using vir_uvm_phase_callback = void (*)(void* component);
void vir_uvm_component_register(const char* type, const char* phase,
                                vir_uvm_phase_callback callback)
    asm("vir.runtime.uvm.component.register");
void vir_uvm_component_register_process(const char* type,
                                        vir_uvm_process_step step)
    asm("vir.runtime.uvm.component.register_process");
void vir_uvm_component_run(const char* phase)
    asm("vir.runtime.uvm.component.run");
void vir_uvm_run_test(const char* type)
    asm("vir.runtime.uvm.run_test");
void vir_uvm_tlm_connect(const char* port, const char* endpoint)
    asm("vir.runtime.uvm.tlm.connect");
void vir_uvm_tlm_write_ptr(const char* port, void* transaction)
    asm("vir.runtime.uvm.tlm.write.ptr");
using vir_uvm_tlm_subscriber = void (*)(void*);
void vir_uvm_tlm_subscribe(const char* endpoint, vir_uvm_tlm_subscriber callback)
    asm("vir.runtime.uvm.tlm.subscribe");
void vir_uvm_tlm_fifo_push_ptr(const char* fifo, void* transaction)
    asm("vir.runtime.uvm.tlm.fifo.push.ptr");
void* vir_uvm_tlm_fifo_try_get_ptr(const char* fifo)
    asm("vir.runtime.uvm.tlm.fifo.try_get.ptr");

void vir_uvm_coverage_register(const char* bin)
    asm("vir.runtime.uvm.coverage.register");
void vir_uvm_coverage_sample_u64(const char* bin, std::uint64_t value)
    asm("vir.runtime.uvm.coverage.sample.u64");
void vir_uvm_coverage_sample_range_u64(const char* bin, std::uint64_t value,
                                       std::uint64_t low, std::uint64_t high)
    asm("vir.runtime.uvm.coverage.sample.range.u64");
double vir_uvm_coverage_percentage()
    asm("vir.runtime.uvm.coverage.percentage");
void vir_uvm_coverage_report()
    asm("vir.runtime.uvm.coverage.report");
void vir_uvm_report(std::uint32_t severity, const char* id, const char* message,
                    std::int32_t verbosity, const char* file, std::int32_t line)
    asm("vir.runtime.uvm.report");
void vir_uvm_report_summary()
    asm("vir.runtime.uvm.report.summary");
std::uint64_t vir_uvm_error_count()
    asm("vir.runtime.uvm.report.error_count");
void vir_uvm_reset() asm("vir.runtime.uvm.reset");

}
