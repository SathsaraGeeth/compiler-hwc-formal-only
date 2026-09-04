#include "IRGen/Frontend/Statement.h"
#include <stdexcept>
namespace vir::irgen::frontend {
namespace {
std::string callee_name(emul::frontend::SemanticNode call) {
    auto name = call.text("subroutine");
    auto space = name.rfind(' ');
    return space == name.npos ? name : name.substr(space + 1);
}
std::string string_literal(emul::frontend::SemanticNode expression) {
    if (expression.kind() == "Conversion")
        return string_literal(expression.child("operand"));
    return expression.kind() == "StringLiteral" ? expression.text("literal")
                                                   : std::string{};
}
std::string endpoint_name(emul::frontend::SemanticNode expression) {
    while (expression.kind() == "Conversion") expression = expression.child("operand");
    if (expression.kind() == "MemberAccess") {
        auto base = expression.child("value");
        if (base.type().find("uvm_tlm_analysis_fifo") != std::string::npos)
            return symbol_name(base);
    }
    return symbol_name(expression);
}
Value& truth(Context& context, Value& value) {
    if (value.type().kind() == Type::Kind::Integer && value.type().width() == 1)
        return value;
    if (value.type().kind() != Type::Kind::Integer)
        throw std::runtime_error("VIR condition must be integer-like");
    auto& zero = context.builder().core().create_constant(
        value.type(), Attribute(std::int64_t{0})).result();
    return context.builder().core().create(
        "binary", {&value, &zero}, {Type::integer(1)},
        {{"operator", "Inequality"}}).result();
}
}

void StatementLowerer::begin_process() {
    if (!context_.self()) throw std::logic_error("process function requires a handle");
    process_ = true;
    auto& body = context_.create_block("process.start");
    context_.set_insertion_point(body);
    resume_states_.push_back({0, &body});
}

void StatementLowerer::finish_process() {
    if (!process_) return;
    if (!context_.terminated()) {
        context_.builder().core().create(
            "call", {context_.self()}, {},
            {{"callee", "vir.runtime.uvm.process.complete"}});
        context_.builder().core().create_return();
    }
    auto& entry = context_.entry();
    context_.set_insertion_point(entry);
    auto& pc = context_.builder().core().create(
        "call", {context_.self()}, {Type::integer(64)},
        {{"callee", "vir.runtime.uvm.process.pc"}}).result();
    for (std::size_t index = 0; index < resume_states_.size(); ++index) {
        auto [state, target] = resume_states_[index];
        auto& expected = context_.builder().core().create_constant(
            Type::integer(64), Attribute(static_cast<std::int64_t>(state))).result();
        auto& equal = context_.builder().core().create(
            "binary", {&pc, &expected}, {Type::integer(1)},
            {{"operator", "Equality"}}).result();
        auto& invalid = context_.create_block("process.dispatch");
        context_.builder().core().create_cond_branch(equal, *target, invalid);
        context_.set_insertion_point(invalid);
    }
    context_.builder().core().create(
        "call", {context_.self()}, {},
        {{"callee", "vir.runtime.uvm.process.complete"}});
    context_.builder().core().create_return();
}

void StatementLowerer::lower(emul::frontend::SemanticNode node) {
    if (!node || node.kind() == "Empty") return;
    if (node.kind() == "VariableDeclaration") {
        if (!process_) return;
        auto variable = node.child("symbol");
        if (!variable) variable = node;
        auto name = symbol_name(variable);
        if (name.empty()) name = node.name();
        if (name.empty()) return;
        locals_.insert(name);
        auto initializer = variable.child("initializer");
        Value* value = initializer ? &expressions_.lower(initializer) : nullptr;
        if (!value) {
            auto type = lower_type(variable);
            value = type.kind() == Type::Kind::Pointer
                ? &context_.builder().core().create("null", {}, {type}).result()
                : &context_.builder().core().create_constant(
                    type, Attribute(std::int64_t{0})).result();
        }
        expressions_.store(variable, *value);
        return;
    }
    if (node.kind() == "Block") return lower(node.child("body"));
    if (node.kind() == "List") {
        for (auto item : node.children("list")) {
            if (context_.terminated()) break;
            lower(item);
        }
        return;
    }
    if (node.kind() == "RepeatLoop") {
        auto count_text = constant_text(node.child("count"));
        if (count_text.empty())
            throw std::runtime_error("VIR repeat loop requires a constant count");
        auto count = parse_integer(count_text);
        if (count < 0 || count > 100000)
            throw std::runtime_error("VIR repeat loop count is outside the supported range");
        for (std::int64_t index = 0; index < count; ++index) lower(node.child("body"));
        return;
    }
    if (node.kind() == "ForLoop") {
        for (auto initializer : node.children("initializers"))
            expressions_.lower(initializer);
        auto& header = context_.create_block("for.cond");
        auto& body = context_.create_block("for.body");
        auto& step = context_.create_block("for.step");
        auto& exit = context_.create_block("for.exit");
        context_.branch(header);
        context_.set_insertion_point(header);
        auto& condition_value = expressions_.lower(node.child("stopExpr"));
        auto& condition = truth(context_, condition_value);
        context_.builder().core().create_cond_branch(condition, body, exit);
        context_.set_insertion_point(body);
        break_targets_.push_back(&exit);
        lower(node.child("body"));
        break_targets_.pop_back();
        context_.branch(step);
        context_.set_insertion_point(step);
        for (auto expression : node.children("steps")) expressions_.lower(expression);
        context_.branch(header);
        context_.set_insertion_point(exit);
        return;
    }
    if (node.kind() == "DoWhileLoop") {
        auto& body = context_.create_block("do.body");
        auto& condition_block = context_.create_block("do.cond");
        auto& exit = context_.create_block("do.exit");
        context_.branch(body);
        context_.set_insertion_point(body);
        break_targets_.push_back(&exit);
        lower(node.child("body"));
        break_targets_.pop_back();
        context_.branch(condition_block);
        context_.set_insertion_point(condition_block);
        auto& condition_value = expressions_.lower(node.child("cond"));
        auto& condition = truth(context_, condition_value);
        context_.builder().core().create_cond_branch(condition, body, exit);
        context_.set_insertion_point(exit);
        return;
    }
    if (node.kind() == "ForeverLoop") {
        auto& loop = context_.create_block("forever.body");
        auto& exit = context_.create_block("forever.exit");
        context_.branch(loop);
        context_.set_insertion_point(loop);
        break_targets_.push_back(&exit);
        const auto resume_count = resume_states_.size();
        lower(node.child("body"));
        break_targets_.pop_back();
        // A suspending body already yields cooperatively.  Adding another timed
        // yield changes edge alignment (and can make monitors miss every valid
        // pulse).  Only protect genuinely non-suspending infinite loops here.
        if (process_ && resume_states_.size() == resume_count &&
            !context_.terminated()) {
            auto state = next_resume_state_++;
            auto& state_value = context_.builder().core().create_constant(
                Type::integer(64), Attribute(static_cast<std::int64_t>(state))).result();
            auto& one = context_.builder().core().create_constant(
                Type::integer(64), Attribute(std::int64_t{1})).result();
            context_.builder().core().create(
                "call", {context_.self(), &state_value}, {},
                {{"callee", "vir.runtime.uvm.process.set_pc"}});
            context_.builder().core().create(
                "call", {context_.self(), &one}, {},
                {{"callee", "vir.runtime.uvm.process.wait_time"}});
            context_.builder().core().create_return();
            auto& continuation = context_.create_block("forever.resume");
            resume_states_.push_back({state, &continuation});
            context_.set_insertion_point(continuation);
        }
        context_.branch(loop);
        context_.set_insertion_point(exit);
        return;
    }
    if (node.kind() == "Break") {
        if (break_targets_.empty())
            throw std::runtime_error("break used outside a VIR loop");
        context_.branch(*break_targets_.back());
        return;
    }
    if (node.kind() == "Return") {
        if (process_)
            context_.builder().core().create(
                "call", {context_.self()}, {},
                {{"callee", "vir.runtime.uvm.process.complete"}});
        context_.builder().core().create_return();
        return;
    }
    if (node.kind() == "Conditional") {
        auto conditions = node.children("conditions");
        if (conditions.size() != 1)
            throw std::runtime_error("VIR conditional requires one normalized condition");
        auto& condition_value = expressions_.lower(conditions.front().child("expr"));
        auto& condition = truth(context_, condition_value);
        auto& yes = context_.create_block("if.then");
        auto& no = context_.create_block("if.else");
        auto& merge = context_.create_block("if.end");
        context_.builder().core().create_cond_branch(condition, yes, no);
        context_.set_insertion_point(yes);
        lower(node.child("ifTrue"));
        context_.branch(merge);
        context_.set_insertion_point(no);
        lower(node.child("ifFalse"));
        context_.branch(merge);
        context_.set_insertion_point(merge);
        return;
    }
    if (node.kind() == "Timed") {
        auto timing = node.child("timing");
        Value* duration = nullptr;
        if (timing.kind() == "Delay") {
            auto expression = timing.child("expr");
            auto literal = constant_text(expression);
            if (!literal.empty())
                duration = &context_.builder().core().create_constant(
                    Type::integer(32), Attribute(parse_integer(literal))).result();
            else
                duration = &expressions_.lower(expression);
        } else if (timing.kind() == "SignalEvent") {
            duration = &context_.builder().core().create_constant(
                Type::integer(32), Attribute(std::int64_t{0})).result();
        } else {
            throw std::runtime_error("unsupported timing: " + timing.kind());
        }
        if (process_) {
            auto state = next_resume_state_++;
            auto& state_value = context_.builder().core().create_constant(
                Type::integer(64), Attribute(static_cast<std::int64_t>(state))).result();
            context_.builder().core().create(
                "call", {context_.self(), &state_value}, {},
                {{"callee", "vir.runtime.uvm.process.set_pc"}});
            if (timing.kind() == "Delay") {
                Value* wait_duration = duration;
                if (duration->type() != Type::integer(64))
                    wait_duration = &context_.builder().core().create(
                        "zext", {duration}, {Type::integer(64)},
                        {{"to", Type::integer(64)}}).result();
                context_.builder().core().create(
                    "call", {context_.self(), wait_duration}, {},
                    {{"callee", "vir.runtime.uvm.process.wait_time"}});
            } else {
                auto event_name = symbol_name(timing.child("expr"));
                auto& event = context_.builder().core().create(
                    "string", {}, {Type::pointer()}, {{"value", event_name}}).result();
                context_.builder().core().create(
                    "call", {context_.self(), &event}, {},
                    {{"callee", "vir.runtime.uvm.process.wait_event"}});
            }
            context_.builder().core().create_return();
            auto& continuation = context_.create_block("process.resume");
            resume_states_.push_back({state, &continuation});
            context_.set_insertion_point(continuation);
        } else {
            context_.builder().intrinsic(Intrinsic::ID::time_delay, {duration});
        }
        lower(node.child("stmt")); return;
    }
    if (node.kind() == "ExpressionStatement") {
        auto expression = node.child("expr");
        if (expression.kind() == "Assignment") { expressions_.lower(expression); return; }
        if (expression.kind() == "UnaryOp") { expressions_.lower(expression); return; }
        if (expression.kind() == "Call" && expression.text("subroutine").ends_with("$finish")) {
            context_.builder().core().create("finish"); return;
        }
        if (expression.kind() == "Call") {
            const auto callee = callee_name(expression);
            if (callee == "$dumpfile" || callee == "$dumpvars" ||
                callee == "build_phase" ||
                callee == "connect_phase" || callee == "run_phase" ||
                callee == "check_phase")
                return;
            if (callee.starts_with("uvm_report_")) {
                const auto severity = callee == "uvm_report_warning" ? 1
                    : callee == "uvm_report_error" ? 2
                    : callee == "uvm_report_fatal" ? 3 : 0;
                auto arguments = expression.children("arguments");
                auto& severity_value = context_.builder().core().create_constant(
                    Type::integer(32), Attribute(std::int64_t{severity})).result();
                auto make_string = [&](std::string value) -> Value& {
                    return context_.builder().core().create(
                        "string", {}, {Type::pointer()}, {{"value", std::move(value)}}).result();
                };
                auto make_integer = [&](std::int64_t value) -> Value& {
                    return context_.builder().core().create_constant(
                        Type::integer(32), Attribute(value)).result();
                };
                auto* id = arguments.size() > 0 ? &expressions_.lower(arguments[0])
                                                : &make_string("UVM");
                auto* message = arguments.size() > 1 ? &expressions_.lower(arguments[1])
                                                     : &make_string("");
                auto* verbosity = arguments.size() > 2 ? &expressions_.lower(arguments[2])
                                                       : &make_integer(0);
                auto* file = arguments.size() > 3 ? &expressions_.lower(arguments[3])
                                                  : &make_string("");
                auto* line = arguments.size() > 4 ? &expressions_.lower(arguments[4])
                                                  : &make_integer(0);
                context_.builder().core().create(
                    "call", {&severity_value, id, message, verbosity, file, line}, {},
                    {{"callee", "vir.runtime.uvm.report"}});
                return;
            }
            if (callee == "run_test") {
                auto arguments = expression.children("arguments");
                auto test = arguments.empty() ? std::string{}
                                              : string_literal(arguments.front());
                if (test.empty()) test = configured_test_;
                if (test.empty())
                    throw std::runtime_error(
                        "run_test requires a test name or set_uvm_test");
                auto& type = context_.builder().core().create(
                    "string", {}, {Type::pointer()}, {{"value", test}}).result();
                context_.builder().core().create(
                    "call", {&type}, {}, {{"callee", "vir.runtime.uvm.run_test"}});
                return;
            }
            if (callee == "raise_objection" || callee == "drop_objection") {
                context_.builder().intrinsic(
                    callee == "raise_objection"
                        ? Intrinsic::ID::uvm_objection_raise
                        : Intrinsic::ID::uvm_objection_drop,
                    {});
                return;
            }
            if (callee == "connect") {
                auto arguments = expression.children("arguments");
                if (arguments.empty())
                    throw std::runtime_error("TLM connect requires an endpoint");
                auto source_name = endpoint_name(expression.child("thisClass"));
                auto endpoint = endpoint_name(arguments.front());
                // Sequencer pull-port connectivity is represented by the
                // sequencer service, not the analysis TLM graph.
                if (source_name == "seq_item_port") return;
                auto& source = context_.builder().core().create(
                    "string", {}, {Type::pointer()}, {{"value", source_name}}).result();
                auto& target = context_.builder().core().create(
                    "string", {}, {Type::pointer()}, {{"value", endpoint}}).result();
                context_.builder().core().create(
                    "call", {&source, &target}, {},
                    {{"callee", "vir.runtime.uvm.tlm.connect"}});
                return;
            }
            if (process_ && callee == "write") {
                auto arguments = expression.children("arguments");
                if (arguments.empty())
                    throw std::runtime_error("analysis write requires a transaction");
                auto& port = context_.builder().core().create(
                    "string", {}, {Type::pointer()},
                    {{"value", endpoint_name(expression.child("thisClass"))}}).result();
                auto& transaction = expressions_.lower(arguments.front());
                context_.builder().core().create(
                    "call", {&port, &transaction}, {},
                    {{"callee", "vir.runtime.uvm.tlm.write.ptr"}});
                return;
            }
            if (callee == "sample" && coverage_groups_) {
                auto group_type = expression.child("thisClass").type();
                if (auto space = group_type.find(' '); space != group_type.npos)
                    group_type.resize(space);
                auto group = coverage_groups_->find(group_type);
                if (group == coverage_groups_->end())
                    throw std::runtime_error("unknown covergroup type " + group_type);
                for (const auto& point : group->second) {
                    auto& sampled = expressions_.lower(point.expression);
                    Value* value = &sampled;
                    if (sampled.type() != Type::integer(64))
                        value = &context_.builder().core().create(
                            "zext", {&sampled}, {Type::integer(64)},
                            {{"to", Type::integer(64)}}).result();
                    for (const auto& bin_model : point.bins) {
                        for (auto range : bin_model.values) {
                            auto low_text = constant_text(range);
                            auto high_text = low_text;
                            if (range.kind() == "ValueRange") {
                                low_text = constant_text(range.child("left"));
                                high_text = constant_text(range.child("right"));
                            }
                            if (low_text.empty() || high_text.empty())
                                throw std::runtime_error(
                                    "non-constant coverage bin " + bin_model.name);
                            auto& bin = context_.builder().core().create(
                                "string", {}, {Type::pointer()},
                                {{"value", bin_model.name}}).result();
                            auto& low = context_.builder().core().create_constant(
                                Type::integer(64), Attribute(parse_integer(low_text))).result();
                            auto& high = context_.builder().core().create_constant(
                                Type::integer(64), Attribute(parse_integer(high_text))).result();
                            context_.builder().core().create(
                                "call", {&bin, value, &low, &high}, {},
                                {{"callee", "vir.runtime.uvm.coverage.sample.range.u64"}});
                        }
                    }
                }
                return;
            }
            if (process_ && (callee == "push_back" || callee == "delete")) {
                auto arguments = expression.children("arguments");
                auto collection_name = symbol_name(expression.child("thisClass"));
                // Slang represents built-in queue methods as free calls whose
                // first argument is the implicit queue object.
                std::size_t argument_index = 0;
                if (collection_name.empty() && !arguments.empty()) {
                    collection_name = symbol_name(arguments.front());
                    argument_index = 1;
                }
                if (collection_name.empty())
                    throw std::runtime_error(callee + " requires a collection");
                auto& name = context_.builder().core().create(
                    "string", {}, {Type::pointer()},
                    {{"value", collection_name}}).result();
                if (argument_index >= arguments.size())
                    throw std::runtime_error(callee + " requires an argument");
                auto& argument = expressions_.lower(arguments[argument_index]);
                context_.builder().core().create(
                    "call", {context_.self(), &name, &argument}, {},
                    {{"callee", callee == "push_back"
                        ? "vir.runtime.uvm.collection.push.ptr"
                        : "vir.runtime.uvm.collection.erase"}});
                return;
            }
            if (process_ && callee == "get_next_item") {
                auto arguments = expression.children("arguments");
                if (arguments.empty())
                    throw std::runtime_error("get_next_item requires a target");
                auto target = arguments.front();
                if (target.kind() == "Assignment") target = target.child("left");
                while (target.kind() == "Conversion") target = target.child("operand");
                auto state = next_resume_state_++;
                auto& retry = context_.create_block("sequence.get.retry");
                auto& ready = context_.create_block("sequence.get.ready");
                auto& waiting = context_.create_block("sequence.get.wait");
                context_.branch(retry);
                resume_states_.push_back({state, &retry});
                context_.set_insertion_point(retry);
                auto& item = context_.builder().core().create(
                    "call", {}, {Type::pointer()},
                    {{"callee", "vir.runtime.uvm.sequence.try_get.ptr"}}).result();
                auto& null = context_.builder().core().create(
                    "null", {}, {Type::pointer()}).result();
                auto& available = context_.builder().core().create(
                    "binary", {&item, &null}, {Type::integer(1)},
                    {{"operator", "Inequality"}}).result();
                context_.builder().core().create_cond_branch(available, ready, waiting);
                context_.set_insertion_point(waiting);
                auto& state_value = context_.builder().core().create_constant(
                    Type::integer(64), Attribute(static_cast<std::int64_t>(state))).result();
                auto& event = context_.builder().core().create(
                    "string", {}, {Type::pointer()},
                    {{"value", "sequencer.default"}}).result();
                context_.builder().core().create(
                    "call", {context_.self(), &state_value}, {},
                    {{"callee", "vir.runtime.uvm.process.set_pc"}});
                context_.builder().core().create(
                    "call", {context_.self(), &event}, {},
                    {{"callee", "vir.runtime.uvm.process.wait_event"}});
                context_.builder().core().create_return();
                context_.set_insertion_point(ready);
                expressions_.store(target, item);
                return;
            }
            if (process_ && callee == "get") {
                auto arguments = expression.children("arguments");
                if (arguments.empty())
                    throw std::runtime_error("FIFO get requires a target");
                auto target = arguments.front();
                if (target.kind() == "Assignment") target = target.child("left");
                while (target.kind() == "Conversion") target = target.child("operand");
                auto fifo_name = symbol_name(expression.child("thisClass"));
                auto state = next_resume_state_++;
                auto& retry = context_.create_block("fifo.get.retry");
                auto& ready = context_.create_block("fifo.get.ready");
                auto& waiting = context_.create_block("fifo.get.wait");
                context_.branch(retry);
                resume_states_.push_back({state, &retry});
                context_.set_insertion_point(retry);
                auto& name = context_.builder().core().create(
                    "string", {}, {Type::pointer()}, {{"value", fifo_name}}).result();
                auto& item = context_.builder().core().create(
                    "call", {&name}, {Type::pointer()},
                    {{"callee", "vir.runtime.uvm.tlm.fifo.try_get.ptr"}}).result();
                auto& null = context_.builder().core().create(
                    "null", {}, {Type::pointer()}).result();
                auto& available = context_.builder().core().create(
                    "binary", {&item, &null}, {Type::integer(1)},
                    {{"operator", "Inequality"}}).result();
                context_.builder().core().create_cond_branch(available, ready, waiting);
                context_.set_insertion_point(waiting);
                auto& state_value = context_.builder().core().create_constant(
                    Type::integer(64), Attribute(static_cast<std::int64_t>(state))).result();
                auto& event = context_.builder().core().create(
                    "string", {}, {Type::pointer()}, {{"value", "tlm." + fifo_name}}).result();
                context_.builder().core().create(
                    "call", {context_.self(), &state_value}, {},
                    {{"callee", "vir.runtime.uvm.process.set_pc"}});
                context_.builder().core().create(
                    "call", {context_.self(), &event}, {},
                    {{"callee", "vir.runtime.uvm.process.wait_event"}});
                context_.builder().core().create_return();
                context_.set_insertion_point(ready);
                expressions_.store(target, item);
                return;
            }
            if (process_ && tasks_ && tasks_->contains(callee)) {
                auto& name = context_.builder().core().create(
                    "string", {}, {Type::pointer()}, {{"value", callee}}).result();
                context_.builder().core().create(
                    "call", {&name, context_.self()}, {Type::pointer()},
                    {{"callee", "vir.runtime.uvm.process.spawn"}});
                return;
            }
            if (callee == "set" && expression.children("arguments").size() == 4) {
                std::vector<Value*> arguments;
                for (auto argument : expression.children("arguments"))
                    if (argument.kind() != "EmptyArgument")
                        arguments.push_back(&expressions_.lower(argument));
                context_.builder().core().create(
                    "call", std::move(arguments), {},
                    {{"callee", "vir.runtime.uvm.config.set.ptr"}});
                return;
            }
            if (callee == "start") {
                auto& sequence = expressions_.lower(expression.child("thisClass"));
                auto arguments = expression.children("arguments");
                Value* sequencer = nullptr;
                if (!arguments.empty()) sequencer = &expressions_.lower(arguments.front());
                if (!sequencer)
                    sequencer = &context_.builder().core().create(
                        "null", {}, {Type::pointer()}).result();
                context_.builder().core().create(
                    "call", {&sequence, sequencer}, {},
                    {{"callee", "vir.runtime.uvm.sequence.start.ptr"}});
                return;
            }
            if (process_ && callee == "start_item") {
                auto arguments = expression.children("arguments");
                if (arguments.empty())
                    throw std::runtime_error("start_item requires a transaction");
                auto& item = expressions_.lower(arguments.front());
                context_.builder().core().create(
                    "call", {context_.self(), &item}, {},
                    {{"callee", "vir.runtime.uvm.sequence.start_item.ptr"}});
                return;
            }
            if (process_ && callee == "finish_item") {
                context_.builder().core().create(
                    "call", {context_.self()}, {},
                    {{"callee", "vir.runtime.uvm.sequence.finish_item"}});
                return;
            }
            if (process_ && callee == "item_done") {
                context_.builder().core().create(
                    "call", {context_.self()}, {},
                    {{"callee", "vir.runtime.uvm.sequence.item_done"}});
                return;
            }
            std::vector<Value*> arguments;
            for (auto argument : expression.children("arguments"))
                if (argument.kind() != "EmptyArgument")
                    arguments.push_back(&expressions_.lower(argument));
            context_.builder().core().create(
                "call", std::move(arguments), {},
                {{"callee", callee}});
            return;
        }
        throw std::runtime_error("unsupported expression statement: " + expression.kind());
    }
    if (node.kind() == "ImmediateAssertion") {
        auto& condition = expressions_.lower(node.child("cond"));
        context_.builder().intrinsic(Intrinsic::ID::assert_check, {&condition});
        return;
    }
    throw std::runtime_error("unsupported frontend VIR statement: " + node.kind());
}
}
