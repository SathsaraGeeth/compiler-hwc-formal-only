#include "VIR/Printer.h"
#include "VIR/Module.h"
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <unordered_map>

namespace vir {
namespace {
std::string llvm_type(const Type& type) {
    return type.kind() == Type::Kind::Object ? "ptr" : type.str();
}

std::string text(const Operation& operation, const std::string& name) {
    auto* attribute = operation.attribute(name);
    if (!attribute) return {};
    auto* value = std::get_if<std::string>(&attribute->value());
    return value ? *value : std::string{};
}

std::int64_t integer(const Operation& operation, const std::string& name) {
    auto* attribute = operation.attribute(name);
    if (!attribute) return 0;
    auto* value = std::get_if<std::int64_t>(&attribute->value());
    return value ? *value : 0;
}

std::string symbol(std::string_view name) {
    std::string escaped;
    for (auto character : name) {
        if (character == '"' || character == '\\') escaped.push_back('\\');
        escaped.push_back(character);
    }
    return "@\"" + escaped + '"';
}

class LLVMPrinter {
public:
    LLVMPrinter(const Module& module, std::ostream& output) : module_(module), output_(output) {}

    void print() {
        collect();
        output_ << "; VIR: portable LLVM IR using the VIR runtime ABI\n";
        for (const auto& [name, type] : globals_)
            output_ << symbol(name) << " = global " << type << " zeroinitializer\n";
        for (const auto& [name, value] : strings_)
            output_ << symbol(name) << " = private unnamed_addr constant [" << value.size() + 1
                    << " x i8] c\"" << value << "\\00\"\n";
        if (!globals_.empty() || !strings_.empty()) output_ << '\n';
        for (const auto& function : module_.functions()) print_function(*function);
        for (const auto& declaration : declarations_) output_ << declaration << '\n';
    }

private:
    void collect() {
        std::size_t next_string = 0;
        for (const auto& function : module_.functions()) {
            for (const auto& block : function->body().blocks()) {
                for (const auto& operation : block->operations()) {
                    auto name = text(*operation, "symbol");
                    if (!name.empty()) {
                        auto type = operation->name() == "load" ? operation->result().type()
                            : operation->operands().front()->type();
                        globals_.try_emplace(name, llvm_type(type));
                    }
                    name = text(*operation, "signal");
                    if (!name.empty()) {
                        Type type = operation->name() == "import" ? operation->result(1).type()
                            : operation->operands().front()->type();
                        globals_.try_emplace(name, llvm_type(type));
                    }
                    name = text(*operation, "instance");
                    if (!name.empty()) strings_.try_emplace(".instance." + name, name);
                    if (operation->name() == "string") {
                        auto value = text(*operation, "value");
                        auto global = ".str." + std::to_string(next_string++);
                        strings_.emplace(global, value);
                        values_[&operation->result()] = "ptr " + symbol(global);
                    }
                    if (operation->name() == "io.read" ||
                        operation->name() == "io.write") {
                        auto io_name = text(*operation, "name");
                        if (!io_name.empty())
                            strings_.try_emplace(".io." + io_name, io_name);
                    }
                }
            }
        }
    }

    std::string value(const Value& value) {
        auto found = values_.find(&value);
        if (found != values_.end()) return found->second;
        auto name = "%" + std::to_string(next_value_++);
        values_[&value] = llvm_type(value.type()) + " " + name;
        return values_[&value];
    }

    std::string reference(const Value& value) {
        auto typed = this->value(value);
        return typed.substr(typed.find(' ') + 1);
    }

    void print_function(const Function& function) {
        if (function.external()) {
            output_ << "declare " << llvm_type(function.type().result()) << ' ' << symbol(function.name())
                    << '(';
            print_types(function.type().elements());
            output_ << ")\n\n";
            return;
        }
        output_ << "define " << llvm_type(function.type().result()) << ' ' << symbol(function.name())
                << '(';
        auto& entry_arguments = function.body().blocks().front()->arguments();
        for (std::size_t index = 0; index < entry_arguments.size(); ++index) {
            if (index) output_ << ", ";
            output_ << value(*entry_arguments[index]);
        }
        output_ << ") {\n";
        for (const auto& block : function.body().blocks()) {
            output_ << (block->name().empty() ? "entry" : block->name()) << ":\n";
            for (const auto& operation : block->operations()) print_operation(*operation);
        }
        output_ << "}\n\n";
    }

    void print_operation(const Operation& operation) {
        auto opcode = operation.name();
        if (opcode == "const") {
            auto* attribute = operation.attribute("value");
            values_[&operation.result()] = llvm_type(operation.result().type()) + " " +
                (attribute ? attribute->str() : "0");
            return;
        }
        if (opcode == "string") return;
        if (opcode == "null") {
            values_[&operation.result()] = llvm_type(operation.result().type()) + " null";
            return;
        }
        if (opcode == "load") {
            auto result = reference(operation.result());
            auto type = llvm_type(operation.result().type());
            output_ << "  " << result << " = load " << type << ", ptr "
                    << symbol(text(operation, "symbol")) << '\n';
            return;
        }
        if (opcode == "store") {
            output_ << "  store " << value(*operation.operands().front()) << ", ptr "
                    << symbol(text(operation, "symbol")) << '\n';
            return;
        }
        if (opcode == "nba_store") {
            declarations_.insert(
                "declare void @\"vir.runtime.nba.store.u64\"(ptr, i64, i32)");
            output_ << "  call void @\"vir.runtime.nba.store.u64\"(ptr "
                    << symbol(text(operation, "symbol")) << ", "
                    << value(*operation.operands().front()) << ", i32 "
                    << integer(operation, "bytes") << ")\n";
            return;
        }
        if (opcode == "binary") return print_binary(operation);
        if (opcode == "unary") return print_unary(operation);
        if (opcode == "concat") return print_concat(operation);
        if (opcode == "trunc" || opcode == "zext") {
            output_ << "  " << reference(operation.result()) << " = " << opcode << ' '
                    << value(*operation.operands().front()) << " to "
                    << llvm_type(operation.result().type()) << '\n';
            return;
        }
        if (opcode == "call") return print_call(operation);
        if (opcode == "io.read") return print_io_read(operation);
        if (opcode == "io.write") return print_io_write(operation);
        if (opcode == "export") return print_export(operation);
        if (opcode == "import") return print_import(operation);
        if (opcode == "evaluate_dut") return print_evaluate(operation);
        if (opcode == "finish") {
            declarations_.insert("declare void @\"vir.runtime.finish\"()");
            output_ << "  call void @\"vir.runtime.finish\"()\n";
            return;
        }
        if (opcode == "ret") { output_ << "  ret void\n"; return; }
        if (opcode == "br") {
            output_ << "  br label %" << text(operation, "target") << '\n'; return;
        }
        if (opcode == "cond_br") {
            output_ << "  br " << value(*operation.operands().front()) << ", label %"
                    << text(operation, "true") << ", label %" << text(operation, "false") << '\n';
            return;
        }
    }

    void print_binary(const Operation& operation) {
        static const std::map<std::string, std::string> names = {
            {"Add", "add"}, {"Subtract", "sub"}, {"Multiply", "mul"},
            {"BitwiseAnd", "and"}, {"BitwiseOr", "or"}, {"BitwiseXor", "xor"},
            {"LogicalAnd", "and"}, {"LogicalOr", "or"}};
        auto operation_name = text(operation, "operator");
        auto result = reference(operation.result());
        auto left = value(*operation.operands()[0]);
        auto right = reference(*operation.operands()[1]);
        static const std::map<std::string, std::string> comparisons = {
            {"Equality", "eq"}, {"Inequality", "ne"},
            {"LessThan", "slt"}, {"LessThanEqual", "sle"},
            {"GreaterThan", "sgt"}, {"GreaterThanEqual", "sge"}};
        if (auto comparison = comparisons.find(operation_name);
            comparison != comparisons.end()) {
            const auto floating = operation.operands()[0]->type().kind() == Type::Kind::Float;
            auto predicate = comparison->second;
            if (floating) {
                if (predicate == "eq") predicate = "oeq";
                else if (predicate == "ne") predicate = "one";
                else predicate = "o" + predicate.substr(1);
            }
            output_ << "  " << result << " = " << (floating ? "fcmp " : "icmp ")
                    << predicate << ' ' << left << ", " << right << '\n';
        } else {
            output_ << "  " << result << " = " << names.at(operation_name) << ' '
                    << left << ", " << right << '\n';
        }
    }

    void print_unary(const Operation& operation) {
        auto result = reference(operation.result());
        auto operand = value(*operation.operands().front());
        if (text(operation, "operator") == "LogicalNot")
            output_ << "  " << result << " = xor " << operand << ", true\n";
        else
            output_ << "  " << result << " = xor " << operand << ", -1\n";
    }

    void print_concat(const Operation& operation) {
        auto result_type = llvm_type(operation.result().type());
        auto result = reference(operation.result());
        auto high = reference(*operation.operands()[0]);
        auto low = reference(*operation.operands()[1]);
        auto high_type = llvm_type(operation.operands()[0]->type());
        auto low_type = llvm_type(operation.operands()[1]->type());
        auto stem = "%concat." + std::to_string(next_temporary_++);
        output_ << "  " << stem << ".high = zext " << high_type << ' ' << high << " to " << result_type << '\n'
                << "  " << stem << ".shift = shl " << result_type << ' ' << stem << ".high, "
                << operation.operands()[1]->type().width() << '\n'
                << "  " << stem << ".low = zext " << low_type << ' ' << low << " to " << result_type << '\n'
                << "  " << result << " = or " << result_type << ' ' << stem << ".shift, " << stem << ".low\n";
    }

    void print_call(const Operation& operation) {
        auto name = text(operation, "callee");
        auto result_type = operation.result_count()
            ? llvm_type(operation.result().type()) : std::string("void");
        std::ostringstream signature;
        signature << "declare " << result_type << ' ' << symbol(name) << '(';
        for (std::size_t i = 0; i < operation.operands().size(); ++i) {
            if (i) signature << ", "; signature << llvm_type(operation.operands()[i]->type());
        }
        signature << ')';
        if (!module_.find_function(name)) declarations_.insert(signature.str());
        output_ << "  ";
        if (operation.result_count()) output_ << reference(operation.result()) << " = ";
        output_ << "call " << result_type << ' ' << symbol(name) << '(';
        print_operands(operation.operands()); output_ << ")\n";
    }

    void print_io_read(const Operation& operation) {
        auto type = llvm_type(operation.result().type());
        auto name = "vir.runtime.io.read." + type;
        declarations_.insert("declare " + type + ' ' + symbol(name) + "(i32, ptr)");
        output_ << "  " << reference(operation.result()) << " = call " << type << ' ' << symbol(name)
                << "(i32 " << integer(operation, "fd") << ", ptr "
                << symbol(".io." + text(operation, "name")) << ")\n";
    }

    void print_io_write(const Operation& operation) {
        auto type = llvm_type(operation.operands().front()->type());
        auto name = "vir.runtime.io.write." + type;
        declarations_.insert("declare void " + symbol(name) +
            "(i32, ptr, " + type + ", " + type + ", " + type + ")");
        output_ << "  call void " << symbol(name) << "(i32 " << integer(operation, "fd")
                << ", ptr " << symbol(".io." + text(operation, "name"))
                << ", " << value(*operation.operands()[0])
                << ", " << value(*operation.operands()[1])
                << ", " << value(*operation.operands()[2]) << ")\n";
    }

    void print_export(const Operation& operation) {
        auto type = llvm_type(operation.operands().front()->type());
        auto name = "vir.runtime.transport.export." + type;
        declarations_.insert("declare i32 " + symbol(name) + "(ptr, " + type + ")");
        output_ << "  " << reference(operation.result()) << " = call i32 " << symbol(name)
                << "(ptr " << symbol(text(operation, "signal")) << ", "
                << value(*operation.operands().front()) << ")\n";
    }

    void print_import(const Operation& operation) {
        auto type = llvm_type(operation.result(1).type());
        auto name = "vir.runtime.transport.import." + type;
        auto temporary = "%import." + std::to_string(next_temporary_++);
        auto status_value = temporary + ".status";
        auto data = temporary + ".data";
        auto xmask = temporary + ".xmask";
        auto zmask = temporary + ".zmask";
        declarations_.insert("declare i64 " + symbol(name) + "(ptr, i1)");
        output_ << "  " << temporary << " = call i64 " << symbol(name)
                << "(ptr " << symbol(text(operation, "signal")) << ", "
                << value(*operation.operands().front()) << ")\n";
        output_ << "  " << status_value << " = and i64 " << temporary << ", 255\n"
                << "  " << reference(operation.result(0)) << " = trunc i64 "
                << status_value << " to i32\n"
                << "  " << data << " = lshr i64 " << temporary << ", 8\n"
                << "  " << reference(operation.result(1)) << " = trunc i64 "
                << data << " to " << type << "\n"
                << "  " << xmask << " = lshr i64 " << temporary << ", 16\n"
                << "  " << reference(operation.result(2)) << " = trunc i64 "
                << xmask << " to " << type << "\n"
                << "  " << zmask << " = lshr i64 " << temporary << ", 24\n"
                << "  " << reference(operation.result(3)) << " = trunc i64 "
                << zmask << " to " << type << "\n";
    }

    void print_evaluate(const Operation& operation) {
        auto aggregate = std::string("{i32, ptr}");
        auto temporary = "%evaluate." + std::to_string(next_temporary_++);
        auto name = "vir.runtime.transport.schedule";
        declarations_.insert("declare " + aggregate + ' ' + symbol(name) + "(ptr)");
        auto instance = ".instance." + text(operation, "instance");
        output_ << "  " << temporary << " = call " << aggregate << ' ' << symbol(name)
                << "(ptr " << symbol(instance) << ")\n"
                << "  " << reference(operation.result(0)) << " = extractvalue " << aggregate << ' '
                << temporary << ", 0\n  " << reference(operation.result(1)) << " = extractvalue "
                << aggregate << ' ' << temporary << ", 1\n";
    }

    void print_operands(const std::vector<Value*>& operands) {
        for (std::size_t i = 0; i < operands.size(); ++i) {
            if (i) output_ << ", "; output_ << value(*operands[i]);
        }
    }
    void print_types(const std::vector<Type>& types) {
        for (std::size_t i = 0; i < types.size(); ++i) {
            if (i) output_ << ", "; output_ << llvm_type(types[i]);
        }
    }

    const Module& module_;
    std::ostream& output_;
    std::map<std::string, std::string> globals_;
    std::map<std::string, std::string> strings_;
    std::unordered_map<const Value*, std::string> values_;
    std::set<std::string> declarations_;
    std::size_t next_value_ = 0;
    std::size_t next_temporary_ = 0;
};
}

void Printer::print(const Module& module, std::ostream& output) {
    LLVMPrinter(module, output).print();
}

std::string Printer::str(const Module& module) {
    std::ostringstream output;
    print(module, output);
    return output.str();
}
}
