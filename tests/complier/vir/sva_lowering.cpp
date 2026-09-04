/* Checks that representative temporal syntax reaches normalized SVA nodes. */

#include "frontend/frontend.h"
#include "vir_/runtime/sva/lowering/directive.h"
#include <stdexcept>

using namespace emul;

namespace {
void require(bool condition) {
    if (!condition)
        throw std::runtime_error("normalized SVA shape mismatch");
}
}

int main(int argc, char** argv) {
    require(argc == 2);
    frontend::FrontendInput input;
    input.top = "sva_shapes";
    input.sources.emplace_back(argv[1]);
    auto design = frontend::Frontend().elaborate(input);

    auto delay = frontend::sva::lower_named_directive(
        design.root(), input.top, "p_delay");
    require(delay.kind == frontend::sva::DirectiveKind::assert_property);
    require(delay.clock.signal == "clk");
    require(delay.clock.edge == frontend::sva::Edge::posedge);
    require(delay.disable.kind == frontend::sva::ExpressionKind::unary);
    require(delay.property.kind == frontend::sva::PropertyKind::implication);

    auto repeat = frontend::sva::lower_named_directive(
        design.root(), input.top, "p_repeat");
    require(repeat.kind == frontend::sva::DirectiveKind::assume_property);
    require(repeat.property.kind == frontend::sva::PropertyKind::implication);

    auto until = frontend::sva::lower_named_directive(
        design.root(), input.top, "p_until");
    require(until.kind == frontend::sva::DirectiveKind::cover_property);
    require(until.property.kind == frontend::sva::PropertyKind::until);
    require(until.property.inclusive);

    auto abort = frontend::sva::lower_named_directive(
        design.root(), input.top, "p_abort");
    require(abort.kind == frontend::sva::DirectiveKind::restrict_property);
    require(abort.property.kind == frontend::sva::PropertyKind::accept_on);

    auto case_property = frontend::sva::lower_named_directive(
        design.root(), input.top, "p_case");
    require(case_property.property.kind ==
            frontend::sva::PropertyKind::case_property);
    require(case_property.property.case_matches.size() == 2);
    require(case_property.property.alternatives.size() == 3);

    auto local = frontend::sva::lower_named_directive(
        design.root(), input.top, "p_local");
    require(local.locals.size() == 1);
    require(local.locals.front().name == "matched");
    require(local.locals.front().width == 32);

}
