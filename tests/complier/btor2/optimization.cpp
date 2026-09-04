#include "HIR/Module.h"
#include "IR/Builder.h"
#include "IR/Verifier.h"
#include "Transform/HIR/PassPipeline.h"
#include "Transform/IR/PassPipeline.h"
#include "Transform/IR/PropertyPartition.h"
#include <cassert>
#include <memory>

using namespace emul;

int main() {
    btor2::Builder builder;
    auto input = builder.input(8, "input");
    auto zero = builder.constant(8, "00000000");
    auto add = builder.binary("add", input, zero);
    auto duplicate = builder.binary("add", input, zero);
    auto four = builder.constant(8, "00000100");
    auto product = builder.binary("mul", input, four);
    auto inverted = builder.unary("not", input);
    auto restored = builder.unary("not", inverted);
    auto dead = builder.state(8, "dead_state");
    builder.init(dead, zero);
    builder.next(dead, dead);
    builder.output(add, "add");
    builder.output(duplicate, "duplicate");
    builder.output(product, "product");
    builder.output(restored, "restored");

    auto& module = builder.module();
    const auto before = module.operations.size();
    assert(btor2::transform::optimize(module));
    btor2::verify(module);
    assert(module.operations.size() < before);
    assert(module.outputs[0].value == module.outputs[1].value);
    assert(module.outputs[0].value == module.outputs[3].value);
    bool saw_shift = false;
    for (const auto& operation : module.operations) {
        assert(operation.symbol != "dead_state");
        assert(operation.opcode != "add");
        assert(operation.opcode != "mul");
        saw_shift |= operation.opcode == "sll";
    }
    assert(saw_shift);

    btor2::Builder partition_builder;
    auto condition = partition_builder.input(1, "condition");
    partition_builder.constraint(condition, "environment");
    partition_builder.bad(condition, "safety");
    partition_builder.justice({condition}, "liveness");
    auto safety = btor2::transform::select_properties(
        partition_builder.module(),
        btor2::transform::PropertyPartition::safety);
    auto liveness = btor2::transform::select_properties(
        partition_builder.module(),
        btor2::transform::PropertyPartition::liveness);
    assert(safety.properties.size() == 2);
    assert(safety.properties[1].kind == btor2::PropertyKind::bad);
    assert(liveness.properties.size() == 2);
    assert(liveness.properties[1].kind == btor2::PropertyKind::justice);

    frontend::sva::Property atom;
    atom.kind = frontend::sva::PropertyKind::sequence;
    atom.sequence = std::make_shared<frontend::sva::Sequence>();
    frontend::sva::Property inner;
    inner.kind = frontend::sva::PropertyKind::negation;
    inner.left = std::make_shared<frontend::sva::Property>(atom);
    frontend::sva::Property outer;
    outer.kind = frontend::sva::PropertyKind::negation;
    outer.left = std::make_shared<frontend::sva::Property>(inner);

    btor2::hir::Operation high_operation;
    high_operation.directive.property = outer;
    high_operation.components = {
        {btor2::hir::Opcode::safety, outer},
        {btor2::hir::Opcode::safety, outer}};
    btor2::hir::Module high;
    high.add(std::move(high_operation));
    assert(btor2::transform::optimize(high));
    assert(high.operations[0].directive.property.kind ==
           frontend::sva::PropertyKind::sequence);
    assert(high.operations[0].components.size() == 1);
}
