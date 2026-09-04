/*
 * compiler/btor2/lib/Conversion/EIRToIR.cpp
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

#include "Conversion/EIRToIR.h"
#include "Lowering/Hierarchy.h"
#include "eir/include/IR/Module.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace emul::btor2 {
namespace {
struct Value { NodeId data, x, z; std::uint32_t width; };
struct StateIds { std::size_t data; std::optional<std::size_t> x, z; };

std::string symbol(std::string name) {
    while (!name.empty() && (name.front() == '%' || name.front() == '@'))
        name.erase(name.begin());
    return name;
}

class Encoder {
public:
    explicit Encoder(const eir::Module& module) : module_(module) {}

    Module run() {
        inputs();
        states();
        std::vector<const eir::Operation*> pending;
        for (const auto& operation : module_.operations) pending.push_back(&operation);
        while (!pending.empty()) {
            bool progressed = false;
            for (auto item = pending.begin(); item != pending.end();) {
                if (!ready(**item)) { ++item; continue; }
                lower(**item);
                item = pending.erase(item);
                progressed = true;
            }
            if (!progressed)
                throw std::runtime_error("BTOR2 cannot schedule EIR operation " +
                                         pending.front()->opcode + " " +
                                         pending.front()->operands);
        }
        return std::move(system_);
    }

private:
    NodeId add(std::string opcode, std::uint32_t width,
               std::vector<NodeId> operands = {},
               std::vector<std::uint32_t> immediates = {},
               std::string name = {}) {
        return system_.add({std::move(opcode), width, std::move(operands),
                            std::move(immediates), std::move(name), {}});
    }
    NodeId constant(std::uint32_t width, char bit) {
        return system_.add({"const", width, {}, {}, {}, std::string(width, bit)});
    }
    NodeId unary(std::string op, NodeId a, std::uint32_t width) {
        return add(std::move(op), width, {a});
    }
    NodeId binary(std::string op, NodeId a, NodeId b, std::uint32_t width) {
        return add(std::move(op), width, {a, b});
    }
    NodeId unknown(const Value& value) {
        return binary("or", value.x, value.z, value.width);
    }
    NodeId any(NodeId value, std::uint32_t width) {
        return width == 1 ? value : unary("redor", value, 1);
    }
    NodeId fill(NodeId condition, std::uint32_t width) {
        return add("ite", width, {condition, constant(width, '1'), constant(width, '0')});
    }
    Value literal(const std::string& text, std::uint32_t width) {
        auto quote = text.find('\'');
        auto digits = quote == text.npos ? text : text.substr(quote + 1);
        unsigned step = 4;
        if (digits.starts_with("0x")) digits.erase(0, 2);
        else if (digits.starts_with("0b")) { digits.erase(0, 2); step = 1; }
        std::string data(width, '0'), x(width, '0'), z(width, '0');
        if (digits.find_first_of("xXzZ") != digits.npos) {
            auto& mask = digits.find_first_of("xX") != digits.npos ? x : z;
            std::fill(mask.begin(), mask.end(), '1');
        } else {
            for (char digit : digits) {
                unsigned number = digit <= '9' ? digit - '0' :
                    (digit | 32) - 'a' + 10;
                for (unsigned bit = 0; bit < step; ++bit) {
                    data.erase(data.begin());
                    data.push_back((number >> (step - bit - 1)) & 1 ? '1' : '0');
                }
            }
        }
        return {system_.add({"const", width, {}, {}, {}, std::move(data)}),
                system_.add({"const", width, {}, {}, {}, std::move(x)}),
                system_.add({"const", width, {}, {}, {}, std::move(z)}), width};
    }
    const Value& get(const std::string& name) const {
        auto found = values_.find(name);
        if (found == values_.end()) throw std::runtime_error("unknown EIR value " + name);
        return found->second;
    }
    Value get_or_literal(const std::string& text, std::uint32_t width) {
        auto found = values_.find(text);
        return found == values_.end() ? literal(text, width) : found->second;
    }
    bool ready(const eir::Operation& operation) const {
        auto args = operation.operand_list();
        if (operation.opcode == "commit") return true;
        if (operation.opcode == "yield") {
            return std::all_of(args.begin(), args.end(), [&](const auto& argument) {
                return values_.contains(argument);
            });
        }
        if (operation.opcode == "export_write")
            return args.size() > 1 && values_.contains(args[1]);
        if (operation.opcode == "state_read") return values_.contains(args.at(0));
        if (operation.opcode == "state_write") return values_.contains(args.at(1));
        auto immediate = [&](std::size_t index) {
            return operation.opcode == "slice" && index > 0;
        };
        for (std::size_t index = 0; index < args.size(); ++index) {
            if (immediate(index) || args[index].empty() ||
                (args[index].front() != '%' && args[index].front() != '@')) continue;
            if (!values_.contains(args[index])) return false;
        }
        return true;
    }
    void constrain(const Value& value, const std::string& name) {
        auto overlap = binary("and", value.x, value.z, value.width);
        auto valid = unary("not", any(overlap, value.width), 1);
        system_.properties.push_back({PropertyKind::constraint, valid, name + ".valid"});
    }
    void inputs() {
        for (const auto& input : module_.inputs) {
            auto type = input.value_type(); auto name = symbol(input.name);
            Value value{add("input", type.width(), {}, {}, name + ".data"),
                        constant(type.width(), '0'), constant(type.width(), '0'), type.width()};
            if (type.four_state()) {
                value.x = add("input", type.width(), {}, {}, name + ".xmask");
                value.z = add("input", type.width(), {}, {}, name + ".zmask");
                constrain(value, name);
            }
            values_[input.name] = value;
        }
    }
    void states() {
        for (const auto& state : module_.states) {
            auto type = state.value_type(); auto name = symbol(state.name);
            Value value{add("state", type.width(), {}, {}, name + ".data"),
                        constant(type.width(), '0'), constant(type.width(), '0'), type.width()};
            StateIds ids{system_.states.size(), {}, {}};
            system_.states.push_back({value.data, {}, {}});
            if (type.four_state()) {
                value.x = add("state", type.width(), {}, {}, name + ".xmask");
                ids.x = system_.states.size(); system_.states.push_back({value.x, {}, {}});
                value.z = add("state", type.width(), {}, {}, name + ".zmask");
                ids.z = system_.states.size(); system_.states.push_back({value.z, {}, {}});
            }
            values_[state.name] = value; states_[state.name] = ids;
        }
    }
    Value logic(std::string op, const Value& a, const Value& b) {
        auto ak = unary("not", unknown(a), a.width), bk = unary("not", unknown(b), b.width);
        auto a0 = binary("and", ak, unary("not", a.data, a.width), a.width);
        auto b0 = binary("and", bk, unary("not", b.data, b.width), b.width);
        auto a1 = binary("and", ak, a.data, a.width), b1 = binary("and", bk, b.data, b.width);
        NodeId zero, one;
        if (op == "and") { zero = binary("or", a0, b0, a.width); one = binary("and", a1, b1, a.width); }
        else if (op == "or") { zero = binary("and", a0, b0, a.width); one = binary("or", a1, b1, a.width); }
        else { auto known = binary("and", ak, bk, a.width); one = binary("and", known, binary("xor", a.data, b.data, a.width), a.width); zero = binary("and", known, unary("not", one, a.width), a.width); }
        auto known = binary("or", zero, one, a.width);
        return {one, unary("not", known, a.width), constant(a.width, '0'), a.width};
    }
    Value conservative(std::string op, const Value& a, const Value& b, std::uint32_t width) {
        auto dirty = binary("or", any(unknown(a), a.width), any(unknown(b), b.width), 1);
        return {binary(std::move(op), a.data, b.data, width), fill(dirty, width), constant(width, '0'), width};
    }
    Value resize(const std::string& op, const Value& a, std::uint32_t width) {
        if (op == "trunc") return {add("slice", width, {a.data}, {width - 1, 0}), add("slice", width, {a.x}, {width - 1, 0}), add("slice", width, {a.z}, {width - 1, 0}), width};
        auto amount = width - a.width; auto data_op = op == "zext" ? "uext" : "sext";
        auto mask_op = op == "zext" ? "uext" : "sext";
        return {add(data_op, width, {a.data}, {amount}), add(mask_op, width, {a.x}, {amount}), add(mask_op, width, {a.z}, {amount}), width};
    }
    void lower(const eir::Operation& operation) {
        auto args = operation.operand_list(); auto op = operation.opcode;
        if (op == "commit") return;
        if (op == "yield") {
            for (size_t index = 0; index < args.size(); ++index) {
                const auto& value = get(args[index]);
                auto has_result = index < module_.results.size();
                auto name = has_result ? symbol(module_.results[index].name) :
                    "result_" + std::to_string(index);
                system_.output({value.data, value.width}, name + ".data");
                if (has_result && module_.results[index].value_type().four_state()) {
                    system_.output({value.x, value.width}, name + ".xmask");
                    system_.output({value.z, value.width}, name + ".zmask");
                }
            }
            return;
        }
        if (op == "export_write") {
            const auto& value = get(args.at(1));
            system_.output({value.data, value.width}, symbol(args.at(0)) + ".data");
            return;
        }
        if (op == "instance")
            throw std::runtime_error("BTOR2 hierarchy lowering is not implemented for instance " + args.at(0));
        if (op == "state_read") { values_[operation.result] = get(args.at(0)); return; }
        if (op == "state_write") {
            auto ids = states_.at(args.at(0)); auto value = get(args.at(1));
            system_.states[ids.data].next = value.data;
            if (ids.x) system_.states[*ids.x].next = value.x;
            if (ids.z) system_.states[*ids.z].next = value.z;
            return;
        }
        auto width = operation.type().width(); Value value{};
        if (op == "not") { auto a = get(args[0]); value = {unary("not", a.data, width), unknown(a), constant(width, '0'), width}; }
        else if (op == "and" || op == "or" || op == "xor") {
            auto a = get_or_literal(args[0], width);
            value = logic(op, a, get_or_literal(args[1], a.width));
        }
        else if (op == "add" || op == "sub" || op == "mul" || op == "shl" || op == "lshr" || op == "ashr") {
            auto mapped = op == "shl" ? "sll" : op == "lshr" ? "srl" : op == "ashr" ? "sra" : op;
            value = conservative(mapped, get(args[0]), get_or_literal(args[1], get(args[0]).width), width);
        } else if (op == "eq" || op == "ne" || op == "slt" || op == "sle" || op == "ult" || op == "ule") {
            auto a = get(args[0]); auto b = get_or_literal(args[1], a.width); auto dirty = binary("or", any(unknown(a), a.width), any(unknown(b), b.width), 1);
            auto mapped = op == "ne" ? "neq" : op == "sle" ? "slte" : op == "ule" ? "ulte" : op;
            value = {binary(mapped, a.data, b.data, 1), dirty, constant(1, '0'), 1};
        } else if (op == "mux") {
            auto s = get_or_literal(args[0], 1);
            auto f = get_or_literal(args[1], width);
            auto t = get_or_literal(args[2], width);
            auto dirty = any(unknown(s), s.width);
            auto selected_x = add("ite", width, {s.data, t.x, f.x}); auto selected_z = add("ite", width, {s.data, t.z, f.z});
            value = {add("ite", width, {s.data, t.data, f.data}), add("ite", width, {dirty, constant(width, '1'), selected_x}), add("ite", width, {dirty, constant(width, '0'), selected_z}), width};
        } else if (op == "zext" || op == "sext" || op == "trunc") value = resize(op, get(args[0]), width);
        else if (op == "slice") { auto a = get(args[0]); auto low = std::stoul(args[1]); std::vector<std::uint32_t> range{static_cast<std::uint32_t>(low + width - 1), static_cast<std::uint32_t>(low)}; value = {add("slice", width, {a.data}, range), add("slice", width, {a.x}, range), add("slice", width, {a.z}, range), width}; }
        else if (op == "concat") {
            auto a = get_or_literal(args[0], width);
            auto b = get_or_literal(args[1], width - a.width);
            value = {binary("concat", a.data, b.data, width), binary("concat", a.x, b.x, width), binary("concat", a.z, b.z, width), width};
        }
        else if (op == "redand" || op == "redor" || op == "redxor") { auto a = get(args[0]); value = {unary(op, a.data, 1), any(unknown(a), a.width), constant(1, '0'), 1}; }
        else throw std::runtime_error("unsupported EIR operation for BTOR2: " + op);
        values_[operation.result] = value;
    }

    const eir::Module& module_; Module system_;
    std::unordered_map<std::string, Value> values_;
    std::unordered_map<std::string, StateIds> states_;
};
}

Module lower(const eir::Program& program) {
    auto module = flatten_hierarchy(program);
    return Encoder(module).run();
}
} 
