#include "IRGen/Frontend/Lowering.h"
#include "IRGen/Frontend/Context.h"
#include "IRGen/Frontend/Expression.h"
#include "IRGen/Frontend/Statement.h"
#include "IRGen/Frontend/Transport.h"
#include "frontend/elaborated_design.h"
#include <functional>
#include <stdexcept>
#include <unordered_set>

namespace vir::irgen {
std::unique_ptr<Module> lower_frontend(
    const emul::frontend::ElaboratedDesign& design,
    std::string_view configured_uvm_test) {
    if (!design) throw std::runtime_error("semantic design is unavailable");
    auto module = std::make_unique<Module>("host");
    for (const auto& binding : design.bindings().entries()) {
        for (auto connection : binding.hardware.children("connections")) {
            auto port = connection.child("port");
            auto expression = connection.child("expr");
            if (expression.kind() == "Assignment") expression = expression.child("left");
            auto host_signal = frontend::symbol_name(expression);
            if (port.name().empty() || host_signal.empty()) continue;
            module->add_signal_binding({port.name(), host_signal,
                                        host_signal == "clk"});
        }
    }
    for (auto host : design.host_design().instances()) {
        auto body = host.child("body");
        {
            frontend::Context context(*module, body.name() + ".combinational");
            frontend::ExpressionLowerer expressions(context);
            for (auto member : body.children("members")) {
                if (member.kind() == "ContinuousAssign")
                    expressions.lower(member.child("assignment"));
            }
            context.finish();
        }
        {
            frontend::Context context(*module, body.name() + ".clocked");
            frontend::StatementLowerer statements(context);
            for (auto member : body.children("members")) {
                if (member.kind() != "ProceduralBlock" ||
                    member.text("procedureKind") != "AlwaysFF")
                    continue;
                auto timed = member.child("body");
                if (timed.kind() == "Timed" &&
                    timed.child("timing").kind() == "SignalEvent")
                    statements.lower(timed.child("stmt"));
            }
            context.finish();
        }
        std::size_t initial_index = 0;
        for (auto member : body.children("members")) {
            if (member.kind() != "ProceduralBlock" || member.text("procedureKind") != "Initial")
                continue;
            auto function_name = body.name() + ".initial";
            if (initial_index++) function_name += "." + std::to_string(initial_index - 1);
            frontend::Context context(*module, std::move(function_name));
            frontend::StatementLowerer(context, std::string(configured_uvm_test))
                .lower(member.child("body"));
            context.finish();
        }
        frontend::lower_transport(*module, host, design);
    }
    // UVM phases are ordinary native VIR functions.  run_test lowers to a
    // direct call to the selected class's run_phase, while phase services use
    // the same runtime ABI as other VIR intrinsics.
    std::unordered_set<std::string> task_names;
    std::function<void(emul::frontend::SemanticNode)> collect_tasks;
    collect_tasks = [&](emul::frontend::SemanticNode node) {
        if (node.kind() == "ClassType") {
            auto source = node.text("source_file");
            if (source.starts_with("../external/") || source.empty()) return;
            for (auto member : node.children("members"))
                if (member.kind() == "Subroutine" &&
                    member.text("subroutineKind") == "Task" &&
                    member.name() != "run_phase")
                    task_names.insert(member.name());
            return;
        }
        for (auto member : node.children("members")) collect_tasks(member);
    };
    collect_tasks(design.host_design().root());

    frontend::CoverageGroups coverage_groups;
    std::function<void(emul::frontend::SemanticNode)> collect_coverage_groups;
    collect_coverage_groups = [&](emul::frontend::SemanticNode node) {
        if (node.kind() == "CovergroupType") {
            auto& points = coverage_groups[std::to_string(node.integer("addr"))];
            std::function<void(emul::frontend::SemanticNode)> collect_points;
            collect_points = [&](emul::frontend::SemanticNode member) {
                if (member.kind() == "Coverpoint") {
                    frontend::CoveragePointModel point;
                    point.expression = member.child("expr");
                    if (!point.expression)
                        throw std::runtime_error("coverpoint expression is unavailable");
                    for (auto bin : member.children("members")) {
                        if (bin.kind() != "CoverageBin") continue;
                        point.bins.push_back({member.name() + "." + bin.name(),
                                              bin.children("values")});
                    }
                    points.push_back(std::move(point));
                    return;
                }
                for (auto child : member.children("members")) collect_points(child);
            };
            collect_points(node);
            return;
        }
        for (auto member : node.children("members")) collect_coverage_groups(member);
    };
    collect_coverage_groups(design.host_design().root());

    {
        frontend::Context context(*module, "__uvm_coverage_init");
        std::function<void(emul::frontend::SemanticNode, std::string)> collect_bins;
        collect_bins = [&](emul::frontend::SemanticNode node, std::string prefix) {
            if (node.kind() == "Coverpoint") prefix = node.name();
            if (node.kind() == "CoverageBin") {
                auto name = prefix.empty() ? node.name() : prefix + "." + node.name();
                auto& bin = context.builder().core().create(
                    "string", {}, {Type::pointer()}, {{"value", std::move(name)}}).result();
                context.builder().core().create(
                    "call", {&bin}, {},
                    {{"callee", "vir.runtime.uvm.coverage.register"}});
            }
            for (auto member : node.children("members"))
                collect_bins(member, prefix);
        };
        collect_bins(design.host_design().root(), {});
        context.finish();
    }

    std::function<void(emul::frontend::SemanticNode)> lower_classes;
    lower_classes = [&](emul::frontend::SemanticNode node) {
        if (node.kind() == "ClassType") {
            auto source = node.text("source_file");
            if (source.starts_with("../external/") || source.empty()) return;
            auto members = node.children("members");
            for (const auto* phase : {"build_phase", "connect_phase", "run_phase",
                                      "check_phase"}) {
                frontend::Context context(*module, node.name() + "." + phase, true);
                frontend::StatementLowerer statements(
                    context, {}, &task_names, &coverage_groups);
                if (std::string_view(phase) == "run_phase") statements.begin_process();
                for (auto member : members) {
                    if (member.kind() != "Subroutine" || member.name() != phase)
                        continue;
                    statements.lower(member.child("body"));
                    break;
                }
                if (std::string_view(phase) == "run_phase") statements.finish_process();
                context.finish();
            }
            for (auto member : members) {
                if (member.kind() != "Subroutine" || member.name() != "write" ||
                    member.text("subroutineKind") != "Function")
                    continue;
                frontend::Context context(
                    *module, "__uvm_subscriber." + node.name(), true);
                auto arguments = member.children("arguments");
                for (auto argument : arguments) {
                    if (argument.kind() != "FormalArgument") continue;
                    context.builder().core().create(
                        "store", {context.self()}, {}, {{"symbol", argument.name()}});
                    break;
                }
                frontend::StatementLowerer statements(
                    context, {}, &task_names, &coverage_groups);
                statements.lower(member.child("body"));
                context.finish();
            }
            for (auto member : members) {
                if (member.kind() != "Subroutine" ||
                    member.text("subroutineKind") != "Task" ||
                    member.name() == "run_phase")
                    continue;
                frontend::Context context(
                    *module, "__uvm_task." + member.name(), true);
                frontend::StatementLowerer statements(
                    context, {}, &task_names, &coverage_groups);
                statements.begin_process();
                statements.lower(member.child("body"));
                statements.finish_process();
                context.finish();
            }
            return;
        }
        for (auto member : node.children("members")) lower_classes(member);
    };
    lower_classes(design.host_design().root());
    return module;
}
}
