#include "IRGen/Frontend/Transport.h"
#include "IRGen/Frontend/Context.h"
#include "IRGen/Frontend/Expression.h"
#include "frontend/elaborated_design.h"
#include <stdexcept>
namespace vir::irgen::frontend {
namespace {
struct Marker { bool input; std::string signal; unsigned width; std::int64_t descriptor; };
std::string parameter(emul::frontend::SemanticNode body, std::string_view name) {
    for (auto member : body.children("members")) {
        if (member.kind() != "Parameter" || member.name() != name) continue;
        auto value = constant_text(member.child("initializer"));
        return value.empty() ? member.text("value") : value;
    }
    throw std::runtime_error("runtime marker lacks parameter " + std::string(name));
}
std::vector<Marker> find_markers(emul::frontend::SemanticNode body) {
    std::vector<Marker> markers;
    for (auto member : body.children("members")) {
        if (member.kind() != "Instance") continue;
        auto marker = member.child("body"); auto name = marker.name();
        if (name != "emul_runtime_import" && name != "emul_runtime_export") continue;
        auto connections = member.children("connections");
        if (connections.size() != 1) throw std::runtime_error("runtime marker requires one connection");
        auto signal = connections.front().child("expr");
        if (signal.kind() == "Assignment") signal = signal.child("left");
        markers.push_back({name == "emul_runtime_import", symbol_name(signal),
            static_cast<unsigned>(std::stoul(parameter(marker, "WIDTH"))),
            parse_integer(parameter(marker, "FD"))});
    }
    return markers;
}
}
void lower_transport(Module& module, emul::frontend::SemanticNode host,
                     const emul::frontend::ElaboratedDesign& design) {
    auto markers = find_markers(host.child("body"));
    if (markers.empty()) return;
    Context context(module, host.child("body").name() + ".transport");
    for (auto& marker : markers) if (marker.input) {
        auto& value = context.builder().core().create("io.read", {},
            {Type::integer(marker.width)},
            {{"fd", marker.descriptor}, {"name", marker.signal}}).result();
        context.builder().export_signal(marker.signal, value);
    }
    auto instance = design.bindings().entries().empty() ? std::string("dut")
        : design.bindings().entries().front().hardware.name();
    context.builder().evaluate_dut(std::move(instance));
    for (auto& marker : markers) if (!marker.input) {
        auto& peek = context.builder().core().create_constant(Type::integer(1), Attribute(false)).result();
        auto& imported = context.builder().import_signal(marker.signal, peek, Type::integer(marker.width));
        context.builder().core().create("io.write",
            {&imported.result(1), &imported.result(2), &imported.result(3)}, {},
            {{"fd", marker.descriptor}, {"name", marker.signal}});
    }
    context.finish();
}
}
