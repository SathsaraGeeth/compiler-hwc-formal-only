#include "Analysis/CFG.h"
#include "Analysis/DataFlow.h"
#include "Analysis/DominatorTree.h"
#include "IRGen/IRBuilder.h"
#include "IRGen/IRGen.h"
#include "IRGen/SSABuilder.h"
#include "VIR/Printer.h"
#include "VIR/Verifier.h"
#include "VIR/IntrinsicInst.h"
#include <cassert>

int main()
{
    using namespace vir;
    irgen::ModuleSpec source{"test", {{"identity", "i32", {{"input", "i32"}}, {{"add", {"input",
                                                                                        "input"},
                                     "sum", "i32", {}}, {"ret", {"sum"}, "", "", {}}}}}};
    auto module = irgen::IRGen{}.lower(source);
    assert(Verifier::verify(*module).empty());
    auto printed = Printer::str(*module);
    assert(printed.find("define i32 @\"identity\"") != std::string::npos);

    Module transport("transport");
    irgen::IRBuilder builder(transport);
    auto &fn = builder.core().create_function("drive", Type::function(Type::void_type(), {}));
    auto &entry = fn.entry_block();
    builder.set_insertion_point(entry);
    auto &value = builder.core().create_constant(Type::integer(32), Attribute(std::int64_t{7})).
                  result();
    auto &peek = builder.core().create_constant(Type::integer(1), Attribute(false)).result();
    assert(builder.export_signal("data", value).result_count() == 1);
    assert(builder.import_signal("data", peek, Type::integer(32)).result_count() == 4);
    assert(builder.evaluate_dut("dut").result_count() == 2);
    auto &yes = fn.body().add_block("yes");
    auto &no = fn.body().add_block("no");
    builder.core().create_cond_branch(peek, yes, no);
    builder.set_insertion_point(yes);
    builder.core().create_return();
    builder.set_insertion_point(no);
    builder.core().create_return();
    assert(Verifier::verify(transport).empty());
    analysis::CFG cfg(fn);
    assert(cfg.successors(entry).size() == 2);
    analysis::DominatorTree dom(cfg);
    assert(dom.dominates(entry, yes));
    analysis::DataFlow flow(cfg);
    flow.run_liveness();
    irgen::SSABuilder ssa;
    ssa.write("x", entry, value);
    ssa.add_predecessor(yes, entry);
    ssa.seal(entry);
    ssa.seal(yes);
    assert(ssa.read("x", yes, Type::integer(32)) == &value);

    Module intrinsic_module("intrinsic");
    irgen::IRBuilder intrinsic_builder(intrinsic_module);
    intrinsic_builder.core().create_function("main", Type::function(Type::void_type(), {}));
    auto &duration = intrinsic_builder.core().create_constant(
        Type::integer(64), Attribute(std::int64_t{1})).result();
    auto &intrinsic = intrinsic_builder.intrinsic(Intrinsic::ID::time_delay, {&duration});
    intrinsic_builder.core().create_return();
    assert(IntrinsicInst::classof(intrinsic));
    assert(IntrinsicInst(intrinsic).intrinsic_id() == Intrinsic::ID::time_delay);
    assert(intrinsic_module.find_function("vir.runtime.time.delay"));
    assert(Verifier::verify(intrinsic_module).empty());
}
