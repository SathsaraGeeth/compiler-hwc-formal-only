#include "IRGen/Frontend/Expression.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <stdexcept>

namespace vir::irgen::frontend {
bool ExpressionLowerer::local(emul::frontend::SemanticNode node) const {
    return process_ && *process_ && locals_ && locals_->contains(symbol_name(node));
}

void ExpressionLowerer::store(emul::frontend::SemanticNode target, Value& value) {
    if (!local(target)) {
        context_.builder().core().create(
            "store", {&value}, {}, {{"symbol", symbol_name(target)}});
        return;
    }
    auto& name = context_.builder().core().create(
        "string", {}, {Type::pointer()}, {{"value", symbol_name(target)}}).result();
    if (value.type().kind() == Type::Kind::Pointer) {
        context_.builder().core().create(
            "call", {context_.self(), &name, &value}, {},
            {{"callee", "vir.runtime.uvm.process.store.ptr"}});
        return;
    }
    Value* serialized = &value;
    if (value.type() != Type::integer(64))
        serialized = &context_.builder().core().create(
            "zext", {&value}, {Type::integer(64)},
            {{"to", Type::integer(64)}}).result();
    context_.builder().core().create(
        "call", {context_.self(), &name, serialized}, {},
        {{"callee", "vir.runtime.uvm.process.store.u64"}});
}

std::string symbol_name(emul::frontend::SemanticNode node) {
    auto name = node.text("symbol");
    if (name.empty()) name = node.text("member");
    if (name.empty()) name = node.name();
    auto space = name.rfind(' ');
    return space == name.npos ? name : name.substr(space + 1);
}

std::string constant_text(emul::frontend::SemanticNode node) {
    auto value = node.text("constant");
    if (value.empty()) value = node.text("value");
    if (value.empty() && node.kind() == "Conversion")
        return constant_text(node.child("operand"));
    return value;
}

Type lower_type(emul::frontend::SemanticNode node) {
    auto text = node.type();
    if (node.kind() == "ArbitrarySymbol") return Type::pointer();
    if (text == "real" || text == "realtime" || text == "shortreal")
        return Type::floating(text == "shortreal" ? "float" : "double");
    auto literal = constant_text(node);
    if (!literal.empty() && literal != "null" && !literal.starts_with('"')) {
        std::smatch literal_width;
        static const std::regex sized_literal(R"(^([0-9]+)')");
        if (std::regex_search(literal, literal_width, sized_literal))
            return Type::integer(static_cast<unsigned>(std::stoul(literal_width[1])));
        return Type::integer(32);
    }
    if (text == "null" || text == "string" || text.starts_with("virtual ") ||
        (!text.empty() && std::isdigit(static_cast<unsigned char>(text.front()))))
        return Type::pointer();
    unsigned width = 1;
    std::smatch match;
    static const std::regex range(R"(\[([0-9]+):([0-9]+)\])");
    if (std::regex_search(text, match, range)) {
        auto left = std::stoul(match[1]);
        auto right = std::stoul(match[2]);
        width = static_cast<unsigned>(left > right ? left - right + 1 : right - left + 1);
    } else if (text.starts_with("int") || text.starts_with("integer")) {
        width = 32;
    } else if (text.starts_with("longint") || text.starts_with("time")) {
        width = 64;
    }
    return Type::integer(width);
}
bool object_member(emul::frontend::SemanticNode node) {
    if (node.kind() != "MemberAccess") return false;
    auto type = node.child("value").type();
    return !type.empty() && std::isdigit(static_cast<unsigned char>(type.front()));
}

std::int64_t parse_integer(std::string text) {
    auto quote = text.find('\'');
    if (quote == text.npos) return std::stoll(text);
    auto base = 10;
    auto position = quote + 1;
    if (position < text.size() && (text[position] == 's' || text[position] == 'S')) ++position;
    if (position < text.size()) {
        auto marker = static_cast<char>(std::tolower(text[position++]));
        if (marker == 'h') base = 16;
        else if (marker == 'b') base = 2;
        else if (marker == 'o') base = 8;
    }
    text = text.substr(position);
    std::erase(text, '_');
    return std::stoll(text, nullptr, base);
}

Value& ExpressionLowerer::lower(emul::frontend::SemanticNode node) {
    if (node.kind() == "EmptyArgument")
        return context_.builder().core().create("null", {}, {Type::pointer()}).result();
    if (node.kind() == "Conversion") return lower(node.child("operand"));
    if (node.kind() == "NullLiteral")
        return context_.builder().core().create("null", {}, {Type::pointer()}).result();
    if (node.kind() == "NewClass") {
        auto type = node.type();
        if (auto space = type.rfind(' '); space != type.npos) type.erase(0, space + 1);
        auto& type_value = context_.builder().core().create(
            "string", {}, {Type::pointer()}, {{"value", type}}).result();
        auto& name = context_.builder().core().create(
            "string", {}, {Type::pointer()}, {{"value", type}}).result();
        auto& parent = context_.builder().core().create(
            "null", {}, {Type::pointer()}).result();
        return context_.builder().core().create(
            "call", {&type_value, &name, &parent}, {Type::pointer()},
            {{"callee", "vir.runtime.uvm.factory.create.ptr"}}).result();
    }
    if (node.kind() == "ElementSelect") {
        if (!context_.self())
            throw std::runtime_error("collection selection requires an object context");
        auto collection = node.child("value");
        auto& name = context_.builder().core().create(
            "string", {}, {Type::pointer()},
            {{"value", symbol_name(collection)}}).result();
        auto& index = lower(node.child("selector"));
        return context_.builder().core().create(
            "call", {context_.self(), &name, &index}, {lower_type(node)},
            {{"callee", "vir.runtime.uvm.collection.get.ptr"}}).result();
    }
    if (node.kind() == "StringLiteral") {
        return context_.builder().core().create(
            "string", {}, {Type::pointer()}, {{"value", node.text("literal")}}).result();
    }
    auto literal = constant_text(node);
    if (!literal.empty()) {
        auto type = lower_type(node);
        return context_.builder().core().create_constant(
            type, type.kind() == Type::Kind::Float
                ? Attribute(std::stod(literal))
                : Attribute(parse_integer(literal))).result();
    }
    if (node.kind() == "NamedValue" || node.kind() == "ArbitrarySymbol" ||
        node.kind() == "HierarchicalValue") {
        if (symbol_name(node) == "this" && context_.self()) return *context_.self();
        if (local(node)) {
            auto& name = context_.builder().core().create(
                "string", {}, {Type::pointer()}, {{"value", symbol_name(node)}}).result();
            auto type = lower_type(node);
            if (type.kind() == Type::Kind::Pointer)
                return context_.builder().core().create(
                    "call", {context_.self(), &name}, {type},
                    {{"callee", "vir.runtime.uvm.process.load.ptr"}}).result();
            auto& loaded = context_.builder().core().create(
                "call", {context_.self(), &name}, {Type::integer(64)},
                {{"callee", "vir.runtime.uvm.process.load.u64"}}).result();
            if (type == loaded.type()) return loaded;
            return context_.builder().core().create(
                "trunc", {&loaded}, {type}, {{"to", type}}).result();
        }
        return context_.builder().core().create(
            "load", {}, {lower_type(node)}, {{"symbol", symbol_name(node)}}).result();
    }
    if (node.kind() == "MemberAccess" && object_member(node)) {
        auto& object = lower(node.child("value"));
        auto& field = context_.builder().core().create(
            "string", {}, {Type::pointer()},
            {{"value", symbol_name(node)}}).result();
        auto type = lower_type(node);
        if (type.kind() == Type::Kind::Pointer)
            return context_.builder().core().create(
                "call", {&object, &field}, {Type::pointer()},
                {{"callee", "vir.runtime.uvm.object.load.ptr"}}).result();
        auto& loaded = context_.builder().core().create(
            "call", {&object, &field}, {Type::integer(64)},
            {{"callee", "vir.runtime.uvm.object.load.u64"}}).result();
        if (type == loaded.type()) return loaded;
        return context_.builder().core().create(
            "trunc", {&loaded}, {type}, {{"to", type}}).result();
    }
    if (node.kind() == "MemberAccess")
        return context_.builder().core().create(
            "load", {}, {lower_type(node)}, {{"symbol", symbol_name(node)}}).result();
    if (node.kind() == "BinaryOp") {
        auto& left = lower(node.child("left"));
        auto& right = lower(node.child("right"));
        return context_.builder().core().create(
            "binary", {&left, &right}, {lower_type(node)}, {{"operator", node.text("op")}})
            .result();
    }
    if (node.kind() == "UnaryOp") {
        if (node.text("op") == "Postincrement" ||
            node.text("op") == "Preincrement" ||
            node.text("op") == "Postdecrement" ||
            node.text("op") == "Predecrement") {
            auto operand = node.child("operand");
            auto& old = lower(operand);
            auto& one = context_.builder().core().create_constant(
                old.type(), Attribute(std::int64_t{1})).result();
            auto increment = node.text("op").ends_with("increment");
            auto& updated = context_.builder().core().create(
                "binary", {&old, &one}, {old.type()},
                {{"operator", increment ? "Add" : "Subtract"}}).result();
            store(operand, updated);
            return node.text("op").starts_with("Post") ? old : updated;
        }
        auto& operand = lower(node.child("operand"));
        return context_.builder().core().create(
            "unary", {&operand}, {lower_type(node)}, {{"operator", node.text("op")}})
            .result();
    }
    if (node.kind() == "Call") {
        auto callee = node.text("subroutine");
        if (auto space = callee.rfind(' '); space != callee.npos)
            callee.erase(0, space + 1);
        if (callee == "uvm_get_report_object")
            return context_.builder().core().create(
                "null", {}, {Type::pointer()}).result();
        if (callee == "get_report_verbosity_level" || callee == "get_report_action")
            return context_.builder().core().create_constant(
                Type::integer(32), Attribute(std::int64_t{100})).result();
        if (callee == "size" && context_.self()) {
            auto collection = node.child("thisClass");
            if (symbol_name(collection).empty()) {
                auto arguments = node.children("arguments");
                if (!arguments.empty()) collection = arguments.front();
            }
            auto collection_name = symbol_name(collection);
            if (collection_name.empty())
                throw std::runtime_error("size requires a collection");
            auto& name = context_.builder().core().create(
                "string", {}, {Type::pointer()},
                {{"value", collection_name}}).result();
            return context_.builder().core().create(
                "call", {context_.self(), &name}, {lower_type(node)},
                {{"callee", "vir.runtime.uvm.collection.size"}}).result();
        }
        if (callee == "get")
            return context_.builder().core().create_constant(
                Type::integer(1), Attribute(std::int64_t{1})).result();
        if (callee == "$cast") {
            auto arguments = node.children("arguments");
            if (arguments.size() != 2)
                throw std::runtime_error("$cast requires target and source");
            auto target = arguments[0];
            while (target.kind() == "Conversion") target = target.child("operand");
            if (target.kind() == "Assignment") target = target.child("left");
            auto& source = lower(arguments[1]);
            auto type = target.type();
            if (auto space = type.rfind(' '); space != type.npos)
                type.erase(0, space + 1);
            auto& type_value = context_.builder().core().create(
                "string", {}, {Type::pointer()}, {{"value", type}}).result();
            store(target, source);
            return context_.builder().core().create(
                "call", {&source, &type_value}, {Type::integer(1)},
                {{"callee", "vir.runtime.uvm.object.is_type"}}).result();
        }
        if (callee == "randomize") {
            auto receiver = node.child("thisClass");
            if (!receiver) {
                auto arguments = node.children("arguments");
                if (arguments.empty())
                    throw std::runtime_error("randomize requires an object");
                receiver = arguments.front();
            }
            auto& object = lower(receiver);
            return context_.builder().core().create(
                "call", {&object}, {Type::integer(1)},
                {{"callee", "vir.runtime.uvm.object.randomize"}}).result();
        }
        if (callee == "$urandom_range") {
            auto arguments = node.children("arguments");
            if (arguments.empty())
                throw std::runtime_error("$urandom_range requires a maximum");
            auto& minimum = lower(arguments.front());
            Value* maximum = nullptr;
            if (arguments.size() > 1) maximum = &lower(arguments[1]);
            if (!maximum) maximum = &minimum;
            return context_.builder().core().create(
                "call", {&minimum, maximum}, {lower_type(node)},
                {{"callee", "vir.runtime.uvm.random.range"}}).result();
        }
        if (callee == "$sformatf") {
            auto arguments = node.children("arguments");
            if (arguments.empty())
                throw std::runtime_error("$sformatf requires a format string");
            return lower(arguments.front());
        }
        std::vector<Value*> arguments;
        for (auto argument : node.children("arguments"))
            if (argument.kind() != "EmptyArgument") arguments.push_back(&lower(argument));
        if (callee == "create") {
            if (arguments.size() > 2) arguments.resize(2);
            auto& type = context_.builder().core().create(
                "string", {}, {Type::pointer()}, {{"value", "uvm_object"}}).result();
            arguments.insert(arguments.begin(), &type);
            if (arguments.size() < 3) {
                auto& parent = context_.builder().core().create(
                    "null", {}, {Type::pointer()}).result();
                arguments.push_back(&parent);
            }
            callee = "vir.runtime.uvm.factory.create.ptr";
        }
        if (callee == "$get_coverage")
            callee = "vir.runtime.uvm.coverage.percentage";
        return context_.builder().core().create(
            "call", std::move(arguments), {lower_type(node)},
            {{"callee", std::move(callee)}}).result();
    }
    if (node.kind() == "Concatenation") {
        auto operands = node.children("operands");
        if (operands.empty()) throw std::runtime_error("empty concatenation");
        auto* result = &lower(operands.front());
        for (std::size_t index = 1; index < operands.size(); ++index) {
            auto& operand = lower(operands[index]);
            result = &context_.builder().core().create(
                "concat", {result, &operand}, {lower_type(node)}).result();
        }
        return *result;
    }
    if (node.kind() == "Assignment") {
        auto target = node.child("left");
        auto right = node.child("right");
        Value* value = nullptr;
        auto right_callee = right.text("subroutine");
        if (right.kind() == "Call" && right_callee.ends_with(" create")) {
            auto type = target.type();
            if (auto space = type.rfind(' '); space != type.npos)
                type.erase(0, space + 1);
            auto arguments = right.children("arguments");
            auto* name = arguments.empty() ? nullptr : &lower(arguments.front());
            if (!name)
                name = &context_.builder().core().create(
                    "string", {}, {Type::pointer()}, {{"value", symbol_name(target)}}).result();
            Value* parent = nullptr;
            if (arguments.size() > 1 && arguments[1].kind() != "EmptyArgument")
                parent = &lower(arguments[1]);
            if (!parent)
                parent = &context_.builder().core().create(
                    "null", {}, {Type::pointer()}).result();
            auto& type_value = context_.builder().core().create(
                "string", {}, {Type::pointer()}, {{"value", type}}).result();
            value = &context_.builder().core().create(
                "call", {&type_value, name, parent}, {Type::pointer()},
                {{"callee", "vir.runtime.uvm.factory.create.ptr"}}).result();
        } else {
            value = &lower(right);
        }
        auto target_type = lower_type(target);
        if (value->type() != target_type) {
            auto opcode = value->type().width() > target_type.width() ? "trunc" : "zext";
            value = &context_.builder().core().create(
                opcode, {value}, {target_type}, {{"to", target_type}}).result();
        }
        if (node.boolean("isNonBlocking") && !object_member(target)) {
            Value* stored = value;
            if (value->type() != Type::integer(64))
                stored = &context_.builder().core().create(
                    "zext", {value}, {Type::integer(64)},
                    {{"to", Type::integer(64)}}).result();
            context_.builder().core().create(
                "nba_store", {stored}, {},
                {{"symbol", symbol_name(target)},
                 {"bytes", static_cast<std::int64_t>((target_type.width() + 7) / 8)}});
        } else if (object_member(target)) {
            auto& object = lower(target.child("value"));
            auto& field = context_.builder().core().create(
                "string", {}, {Type::pointer()},
                {{"value", symbol_name(target)}}).result();
            if (value->type().kind() == Type::Kind::Pointer) {
                context_.builder().core().create(
                    "call", {&object, &field, value}, {},
                    {{"callee", "vir.runtime.uvm.object.store.ptr"}});
                return *value;
            }
            Value* stored = value;
            if (value->type() != Type::integer(64))
                stored = &context_.builder().core().create(
                    "zext", {value}, {Type::integer(64)},
                    {{"to", Type::integer(64)}}).result();
            context_.builder().core().create(
                "call", {&object, &field, stored}, {},
                {{"callee", "vir.runtime.uvm.object.store.u64"}});
        } else {
            store(target, *value);
        }
        return *value;
    }
    throw std::runtime_error("unsupported frontend VIR expression: " + node.kind());
}
}
