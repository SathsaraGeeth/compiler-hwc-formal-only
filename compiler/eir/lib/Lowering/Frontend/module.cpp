/*
 * compiler/eir/lib/Lowering/Frontend/module.cpp
 *
 * Copyright (C) 2026 Sathsara Geeth
 */

/*
 * Version 1.0
 *
 * Version History
 *
 * Version | Description
 * --------+-----------------------------------------
 * 1.0     | Initial implementation
 */

/*
 * Comments:
 *
 */

#include "module.h"
#include "context.h"
#include <algorithm>
#include <regex>
#include <stdexcept>

namespace emul::lowering::semantic {
namespace {
size_t memory_size(std::string_view type) {
    static const std::regex unpacked(R"(\$\[([0-9]+):([0-9]+)\])");
    std::cmatch match;
    if (!std::regex_search(type.begin(), type.end(), match, unpacked)) return 0;
    auto left = std::stoul(match[1].str()), right = std::stoul(match[2].str());
    return left > right ? left - right + 1 : right - left + 1;
}

void collect_targets(frontend::SemanticNode node, Context& context) {
    auto kind = node.kind();
    if (kind == "ExpressionStatement") {
        auto expr = node.child("expr");
        if (expr.kind() != "Assignment") return;
        if (!expr.boolean("isNonBlocking")) return;
        auto target = expr.child("left");
        if (target.kind() == "NamedValue" || target.kind() == "HierarchicalValue") {
            auto name = symbol_name(target);
            if (target.kind() == "HierarchicalValue")
                context.types.try_emplace(name, lower_type(target));
            context.states.insert(name);
        }
        else if (target.kind() == "Concatenation") {
            for (auto operand : target.children("operands")) {
                if (operand.kind() != "NamedValue")
                    throw std::runtime_error("unsupported sequential concatenation target");
                context.states.insert(symbol_name(operand));
            }
        }
        else if (target.kind() == "ElementSelect" || target.kind() == "MemberAccess") {
            auto base = target;
            while (base && (base.kind() == "ElementSelect" || base.kind() == "MemberAccess"))
                base = base.child("value");
            auto memory = symbol_name(base);
            auto found = context.memories.find(memory);
            if (found != context.memories.end()) {
                for (size_t index = 0; index < found->second; ++index)
                    context.states.insert(memory + "[" + std::to_string(index) + "]");
            } else if (!memory.empty()) {
                context.states.insert(memory);
            } else {
                throw std::runtime_error("unsupported sequential aggregate target");
            }
        }
        return;
    }
    if (kind == "Block") return collect_targets(node.child("body"), context);
    if (kind == "List") {
        for (auto item : node.children("list")) collect_targets(item, context);
        return;
    }
    if (kind == "Conditional") {
        collect_targets(node.child("ifTrue"), context);
        if (auto other = node.child("ifFalse")) collect_targets(other, context);
        return;
    }
    if (kind == "Case") {
        for (auto item : node.children("items")) collect_targets(item.child("stmt"), context);
        if (auto fallback = node.child("defaultCase")) collect_targets(fallback, context);
        return;
    }
    if (kind == "ForLoop") collect_targets(node.child("body"), context);
}

LoweredValue edge(Context& context, frontend::SemanticNode event) {
    if (event.kind() == "EventList") {
        LoweredValue result{"4s<1>'0x0", "4s<1>"};
        for (auto child : event.children("events")) {
            auto item = edge(context, child);
            result = context.emit("or", result.name + ", " + item.name, "4s<1>");
        }
        return result;
    }
    if (event.kind() != "SignalEvent") throw std::runtime_error("unsupported event control");
    auto signal = context.expression(event.child("expr"));
    auto name = symbol_name(event.child("expr"));
    auto state = name + "_prev";
    if (context.states.insert(state).second)
        context.module.states.push_back({"@" + state, "4s<1>"});
    auto previous = context.read(state, "4s<1>");
    LoweredValue result;
    if (event.text("edge") == "PosEdge") {
        auto inverted = context.emit("not", previous.name, "4s<1>");
        result = context.emit("and", inverted.name + ", " + signal.name, "4s<1>");
    } else if (event.text("edge") == "NegEdge") {
        auto inverted = context.emit("not", signal.name, "4s<1>");
        result = context.emit("and", inverted.name + ", " + previous.name, "4s<1>");
    } else throw std::runtime_error("unsupported signal edge");
    context.module.operations.push_back({{}, {}, "state_write", "@" + state + ", " + signal.name});
    return result;
}

void lower_instance(frontend::SemanticNode instance, Context& context) {
    std::vector<std::string> arguments;
    std::vector<frontend::SemanticNode> outputs;
    for (auto connection : instance.children("connections")) {
        auto port = connection.child("port");
        if (port.text("direction") == "In") {
            auto expression = connection.child("expr");
            try {
                arguments.push_back(context.expression(expression).name);
            } catch (const UnresolvedValue& unresolved) {
                if (expression.kind() != "NamedValue" &&
                    expression.kind() != "HierarchicalValue")
                    throw;
                arguments.push_back("$deferred(" + unresolved.name() + ")");
            }
        } else {
            outputs.push_back(connection.child("expr"));
        }
    }
    std::string results;
    std::vector<LoweredValue> produced;
    for (auto output : outputs) {
        auto value = LoweredValue{context.temporary(), lower_type(output)};
        if (!results.empty()) results += ", ";
        results += value.name;
        produced.push_back(std::move(value));
    }
    std::string operands = "@" + instance.name() + ", @" + instance.child("body").name() + "(";
    for (size_t index = 0; index < arguments.size(); ++index) {
        if (index) operands += ", ";
        operands += arguments[index];
    }
    operands += ")";
    context.module.operations.push_back({results, {}, "instance", std::move(operands)});
    for (size_t index = 0; index < outputs.size(); ++index) {
        auto target = outputs[index].kind() == "Assignment"
            ? outputs[index].child("left")
            : outputs[index];
        if (!target || target.kind() == "EmptyArgument")
            continue;
        context.assign(target, produced[index], nullptr, false);
    }
}

void resolve_deferred_inputs(Context& context) {
    for (const auto& [name, count] : context.memories) {
        if (context.values.contains(name))
            continue;
        LoweredValue packed;
        bool complete = true;
        for (size_t index = count; index-- > 0;) {
            auto item = context.values.find(
                name + "[" + std::to_string(index) + "]");
            if (item == context.values.end()) {
                complete = false;
                break;
            }
            packed = packed.name.empty() ? item->second : context.emit(
                "concat", packed.name + ", " + item->second.name,
                std::string(packed.type.starts_with("4s<") ? "4s<" : "2s<") +
                    std::to_string(type_width(packed.type) +
                                   type_width(item->second.type)) + ">");
        }
        if (complete)
            context.values[name] = packed;
    }
    for (auto& operation : context.module.operations) {
        size_t begin = 0;
        while ((begin = operation.operands.find("$deferred(", begin)) !=
               std::string::npos) {
            auto end = operation.operands.find(')', begin);
            if (end == std::string::npos)
                throw std::runtime_error("invalid deferred instance input");
            auto name = operation.operands.substr(begin + 10, end - begin - 10);
            auto found = context.values.find(name);
            if (found == context.values.end())
                throw UnresolvedValue(name);
            operation.operands.replace(
                begin, end - begin + 1, found->second.name);
            begin += found->second.name.size();
        }
    }
}

eir::Module lower_body(frontend::SemanticNode body) {
    eir::Module module{.name = body.name()};
    Context context(module);
    std::vector<std::string> output_names;
    auto members = body.children("members");
    for (auto member : members) {
        if (member.kind() == "Port") {
            auto type = lower_type(member);
            auto count = memory_size(member.type());
            if (count) {
                auto element_type = member.type();
                auto marker = element_type.rfind("$[");
                if (marker != std::string::npos) element_type.resize(marker);
                context.types[member.name()] = lower_type(element_type);
                context.memories[member.name()] = count;
                auto element_width = type_width(context.types.at(member.name()));
                type = std::string(context.types.at(member.name()).starts_with("4s<") ?
                                   "4s<" : "2s<") +
                       std::to_string(element_width * count) + ">";
            } else {
                context.types[member.name()] = type;
            }
            if (member.text("direction") == "In") {
                module.inputs.push_back({"%" + member.name(), type});
                if (count) {
                    auto element_type = context.types.at(member.name());
                    auto width = type_width(element_type);
                    for (size_t index = 0; index < count; ++index)
                        context.values[member.name() + "[" + std::to_string(index) + "]"] =
                            context.emit("slice", "%" + member.name() + ", " +
                                         std::to_string(index * width) + ", " +
                                         std::to_string(width), element_type);
                } else {
                    context.values[member.name()] = {"%" + member.name(), type};
                }
            } else {
                module.results.push_back({member.name(), type});
                output_names.push_back(member.name());
            }
        } else if (member.kind() == "Variable" || member.kind() == "Net") {
            auto count = memory_size(member.type());
            if (count) {
                auto element_type = member.type();
                auto marker = element_type.rfind("$[");
                if (marker != std::string::npos) element_type.resize(marker);
                context.types[member.name()] = lower_type(element_type);
                context.memories[member.name()] = count;
            } else {
                context.types[member.name()] = lower_type(member);
            }
        }
    }
    bool has_initial = false;
    for (auto member : members) {
        if (member.kind() != "ProceduralBlock") continue;
        auto procedure = member.text("procedureKind");
        if (procedure == "AlwaysFF") {
            auto timed = member.child("body");
            collect_targets(timed.child("stmt"), context);
        } else if (procedure == "Initial") {
            has_initial = true;
            collect_targets(member.child("body"), context);
        }
    }
    if (has_initial) {
        context.types["__initial_done"] = "2s<1>";
        context.states.insert("__initial_done");
    }
    for (const auto& state : context.states) {
        auto bracket = state.find('[');
        auto base = bracket == std::string::npos ? state : state.substr(0, bracket);
        module.states.push_back({"@" + state, context.types.at(base)});
    }
    std::vector<frontend::SemanticNode> combinational;
    for (auto member : members)
        if (member.kind() == "ContinuousAssign" || member.kind() == "Instance" ||
            (member.kind() == "ProceduralBlock" && member.text("procedureKind") == "AlwaysComb"))
            combinational.push_back(member);
    while (!combinational.empty()) {
        bool progress = false;
        std::string unresolved;
        for (size_t index = 0; index < combinational.size();) {
            auto operation_count = module.operations.size();
            auto saved_values = context.values;
            auto saved_pending = context.pending;
            auto saved_next = context.next_value;
            try {
                auto member = combinational[index];
                if (member.kind() == "ContinuousAssign") {
                    auto assignment = member.child("assignment");
                    context.assign(assignment.child("left"),
                                   context.expression(assignment.child("right")), nullptr, false);
                } else if (member.kind() == "ProceduralBlock") {
                    context.statement(member.child("body"), nullptr, false);
                } else {
                    lower_instance(member, context);
                }
                combinational.erase(combinational.begin() + index);
                progress = true;
            } catch (const UnresolvedValue& error) {
                unresolved = error.what();
                module.operations.resize(operation_count);
                context.values = std::move(saved_values);
                context.pending = std::move(saved_pending);
                context.next_value = saved_next;
                ++index;
            }
        }
        if (!progress) throw std::runtime_error("combinational dependency cycle in @" +
                                                module.name + ": " + unresolved);
    }
    resolve_deferred_inputs(context);
    if (has_initial) {
        auto initialized = context.read("__initial_done", "2s<1>");
        auto active = context.emit("not", initialized.name, "2s<1>");
        for (auto member : members)
            if (member.kind() == "ProceduralBlock" &&
                member.text("procedureKind") == "Initial")
                context.statement(member.child("body"), &active, true);
        context.pending["__initial_done"] = {"2s<1>'0x1", "2s<1>"};
    }
    for (auto member : members) {
        if (member.kind() == "ProceduralBlock" && member.text("procedureKind") == "AlwaysFF") {
            auto timed = member.child("body");
            auto enabled = edge(context, timed.child("timing"));
            context.statement(timed.child("stmt"), &enabled, true);
        }
    }
    for (const auto& [name, value] : context.pending)
        module.operations.push_back({{}, {}, "state_write", "@" + name + ", " + value.name});
    std::string yielded;
    for (const auto& name : output_names) {
        LoweredValue value;
        auto memory = context.memories.find(name);
        if (memory != context.memories.end()) {
            auto whole = context.pending.find(name);
            auto combinational = context.values.find(name);
            if (whole != context.pending.end()) {
                value = whole->second;
            } else if (combinational != context.values.end()) {
                value = combinational->second;
            } else {
                auto element_type = context.types.at(name);
                for (size_t index = memory->second; index-- > 0;) {
                    auto item_name = name + "[" + std::to_string(index) + "]";
                    auto pending = context.pending.find(item_name);
                    auto item = pending == context.pending.end()
                        ? context.read(item_name, element_type) : pending->second;
                    value = value.name.empty()
                        ? item
                        : context.emit("concat", value.name + ", " + item.name,
                                       "4s<" + std::to_string(type_width(value.type) +
                                                              type_width(item.type)) + ">");
                }
            }
        } else if (context.values.contains(name + "[0]")) {
            for (size_t index = 0;
                 context.values.contains(name + "[" +
                                         std::to_string(index) + "]");
                 ++index) {
                auto item = context.values.at(
                    name + "[" + std::to_string(index) + "]");
                value = value.name.empty()
                    ? item
                    : context.emit("concat", item.name + ", " + value.name,
                                   "4s<" + std::to_string(type_width(value.type) +
                                                          type_width(item.type)) + ">");
            }
        } else {
            auto pending = context.pending.find(name);
            auto combinational = context.values.find(name);
            if (pending != context.pending.end())
                value = pending->second;
            else if (combinational != context.values.end())
                value = combinational->second;
            else if (context.states.contains(name))
                value = context.read(name, context.types.at(name));
            else {
                value = {context.types.at(name) + "'0xx",
                         context.types.at(name)};
            }
        }
        if (!value.name.starts_with('%'))
            value = context.emit("or", value.name + ", " + value.type + "'0x0", value.type);
        if (!yielded.empty()) yielded += ", ";
        yielded += value.name;
    }
    module.operations.push_back({{}, {}, "yield", yielded});
    return module;
}

}

eir::Module lower_module(frontend::SemanticNode body) {
    return lower_body(body);
}

eir::Program lower_design(const frontend::ElaboratedDesign& design) {
    if (!design) throw std::runtime_error("semantic design is unavailable");
    register_type_aliases(design.detailed_root());
    eir::Program program;
    for (auto instance : design.hardware_design().instances())
        program.modules.push_back(lower_body(instance.child("body")));
    return program;
}
}
