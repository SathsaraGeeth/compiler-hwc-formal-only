/*
 * compiler/tools/main.cpp
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

#include "artifacts.h"
#include "dump.h"
#include "file_list.h"
#include "formal.h"
#include "runtime.h"
#include "words.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace emul::tool {
namespace {
void print_help(std::ostream& output) {
    output <<
        "usage: hwc -f script.tcl [-norun] [COMMAND]\n"
        "       hwc -help\n\n"
        "Script commands:\n"
        "  read_file -f filelist.f\n"
        "  set_top TOP\n"
        "  set_timescale TIMEUNIT/TIMEPRECISION\n"
        "  set_target cpu|fpga\n"
        "  set_uvm_test TEST\n"
        "  set_seed NUMBER\n"
        "  simulate\n"
        "  emulate\n"
        "  write_artifacts DIRECTORY\n"
        "  dump_elb_design\n"
        "  dump_eir_unopt\n"
        "  dump_vir_unopt\n"
        "  dump_eir_opt\n"
        "  dump_vir_opt\n"
        "  dump_mir\n"
        "  dump_btor2_high_unopt\n"
        "  dump_btor2_high_opt\n"
        "  dump_btor2_low_unopt\n"
        "  dump_btor2_low_opt\n"
        "  dump_asm backend=emul|cpu\n"
        "  create_clock SIGNAL\n"
        "  set_reset SIGNAL [-active_low|-active_high]\n"
        "  set_engine auto|bmc|kind|pdr|smt\n"
        "  set_formal_backend auto|pono|btormc|ric3|avr|abc|avy|suprove|smtbmc\n"
        "  set_formal_property PROPERTY\n"
        "  set_formal_task auto|safety|liveness|fairness|reachability|performance\n"
        "  set_max_depth NUMBER\n"
        "  set_formal_work_directory DIRECTORY\n"
        "  set_formal_timeout NUMBER[ms|s|m|h]\n"
        "  set_expected_result default|any|proved|bounded|covered|counterexample|unknown|timeout\n"
        "  set_cover_trace shortest|first\n"
        "  formal_task NAME ENGINE BACKEND\n"
        "  formal_matrix prove|cover PROPERTY\n"
        "  prove -all|PROPERTY\n"
        "  cover -all|PROPERTY\n\n"
        "Physical FPGA execution uses HWC_FPGA_SERVER=HOST:PORT.\n";
}

uint64_t unsigned_number(std::string_view value, std::string_view command) {
    size_t used = 0;
    const auto number = std::stoull(std::string(value), &used, 0);
    if (used != value.size())
        throw std::runtime_error(std::string(command) + " expects an integer");
    return number;
}

uint64_t duration_ms(std::string_view value) {
    uint64_t multiplier = 1;
    auto number = value;
    if (value.ends_with("ms")) number.remove_suffix(2);
    else if (value.ends_with('s')) { number.remove_suffix(1); multiplier = 1000; }
    else if (value.ends_with('m')) { number.remove_suffix(1); multiplier = 60 * 1000; }
    else if (value.ends_with('h')) { number.remove_suffix(1); multiplier = 60 * 60 * 1000; }
    const auto parsed = unsigned_number(number, "set_formal_timeout");
    if (parsed > std::numeric_limits<uint64_t>::max() / multiplier)
        throw std::runtime_error("set_formal_timeout is outside the supported range");
    return parsed * multiplier;
}

void require_count(const std::vector<std::string>& words, size_t expected) {
    if (words.size() != expected)
        throw std::runtime_error(words.front() + " expects " +
                                 std::to_string(expected - 1) + " argument(s)");
}

void execute_command(Session& session, const std::filesystem::path& base,
                     const std::vector<std::string>& words, size_t& actions) {
    if (words.empty()) return;
    const auto& command = words.front();
    if (command == "read_file") {
        require_count(words, 3);
        if (words[1] != "-f")
            throw std::runtime_error("read_file expects -f filelist.f");
        read_file_list(session, base / words[2]);
    } else if (command == "set_top") {
        require_count(words, 2);
        session.top = words[1];
    } else if (command == "set_timescale") {
        require_count(words, 2);
        session.timescale = words[1];
    } else if (command == "set_target") {
        require_count(words, 2);
        if (words[1] == "cpu") session.target = Target::cpu;
        else if (words[1] == "fpga") session.target = Target::fpga;
        else throw std::runtime_error("unknown target: " + words[1]);
    } else if (command == "set_uvm_test") {
        require_count(words, 2);
        session.uvm_test = words[1];
    } else if (command == "set_seed") {
        require_count(words, 2);
        session.seed = unsigned_number(words[1], command);
    } else if (command == "create_clock") {
        require_count(words, 2);
        session.clock = words[1];
        if (std::find(session.clocks.begin(), session.clocks.end(), words[1]) ==
            session.clocks.end())
            session.clocks.push_back(words[1]);
    } else if (command == "set_reset") {
        if (words.size() != 2 && words.size() != 3)
            throw std::runtime_error(
                "set_reset expects SIGNAL [-active_low|-active_high]");
        session.reset = words[1];
        if (words.size() == 3) {
            if (words[2] == "-active_low") session.reset.insert(0, "~");
            else if (words[2] != "-active_high")
                throw std::runtime_error("unknown reset option: " + words[2]);
        }
    } else if (command == "set_engine") {
        require_count(words, 2);
        session.engine = words[1];
    } else if (command == "set_formal_backend") {
        require_count(words, 2);
        session.formal_backend = words[1];
    } else if (command == "set_formal_property") {
        require_count(words, 2);
        session.formal_property = words[1];
    } else if (command == "set_formal_task") {
        require_count(words, 2);
        session.formal_task = words[1];
    } else if (command == "set_max_depth") {
        require_count(words, 2);
        const auto depth = unsigned_number(words[1], command);
        if (!depth || depth > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error(
                "set_max_depth is outside the supported range");
        session.max_depth = static_cast<uint32_t>(depth);
    } else if (command == "set_formal_work_directory") {
        require_count(words, 2);
        session.formal_work_directory = base / words[1];
    } else if (command == "set_formal_timeout") {
        require_count(words, 2);
        session.formal_timeout_ms = duration_ms(words[1]);
    } else if (command == "set_expected_result") {
        require_count(words, 2);
        static const std::vector<std::string> valid = {
            "default", "any", "proved", "bounded", "covered",
            "counterexample", "unknown", "timeout"};
        if (std::find(valid.begin(), valid.end(), words[1]) == valid.end())
            throw std::runtime_error("unknown expected formal result: " + words[1]);
        session.expected_result = words[1];
    } else if (command == "set_cover_trace") {
        require_count(words, 2);
        if (words[1] != "shortest" && words[1] != "first")
            throw std::runtime_error("set_cover_trace expects shortest or first");
        session.cover_trace = words[1];
    } else if (command == "formal_task") {
        require_count(words, 4);
        static const std::vector<std::string> engines = {
            "auto", "bmc", "kind", "pdr", "smt"};
        static const std::vector<std::string> backends = {
            "auto", "pono", "btormc", "ric3", "avr", "abc", "avy",
            "suprove", "smtbmc"};
        if (std::find(engines.begin(), engines.end(), words[2]) == engines.end())
            throw std::runtime_error("unknown formal matrix engine: " + words[2]);
        if (std::find(backends.begin(), backends.end(), words[3]) == backends.end())
            throw std::runtime_error("unknown formal matrix backend: " + words[3]);
        if (std::any_of(session.formal_matrix.begin(), session.formal_matrix.end(),
                        [&](const auto& task) { return task.name == words[1]; }))
            throw std::runtime_error("duplicate formal matrix task: " + words[1]);
        session.formal_matrix.push_back({words[1], words[2], words[3]});
    } else if (command == "formal_matrix") {
        require_count(words, 3);
        if (words[1] != "prove" && words[1] != "cover")
            throw std::runtime_error("formal_matrix expects prove or cover");
        if (session.formal_matrix.empty())
            throw std::runtime_error("formal_matrix has no formal_task entries");
        ++actions;
        if (session.no_run) return;
        const auto action = words[1] == "prove" ? FormalAction::prove
                                                  : FormalAction::cover;
        bool passed = true;
        for (const auto& task : session.formal_matrix) {
            auto task_session = session;
            task_session.engine = task.engine;
            task_session.formal_backend = task.backend;
            task_session.formal_work_directory /= task.name;
            std::cout << "[formal] matrix task: " << task.name << '\n';
            try {
                if (!run_formal(task_session, action, words[2])) passed = false;
            } catch (const std::exception& error) {
                std::cerr << "[formal] matrix task " << task.name
                          << " error: " << error.what() << '\n';
                passed = false;
            }
        }
        if (!passed) throw std::runtime_error("formal matrix failed");
    } else if (command == "simulate" || command == "emulate") {
        require_count(words, 1);
        if (session.no_run) {
            ++actions;
            return;
        }
        const auto passed = command == "simulate" ? simulate(session)
                                                   : emulate(session);
        ++actions;
        if (!passed) throw std::runtime_error(command + " failed");
    } else if (command == "write_artifacts") {
        require_count(words, 2);
        write_artifacts(session, base / words[1]);
        ++actions;
    } else if (command == "dump_elb_design" || command == "-dump_elb_design" ||
               command == "dump_eir_unopt" || command == "-dump_eir_unopt" ||
               command == "dump_vir_unopt" || command == "-dump_vir_unopt" ||
               command == "dump_eir_opt" || command == "-dump_eir_opt" ||
               command == "dump_vir_opt" || command == "-dump_vir_opt" ||
               command == "dump_mir" || command == "-dump_mir") {
        require_count(words, 1);
        auto kind = command;
        if (kind.front() == '-') kind.erase(0, 1);
        kind.erase(0, 5);
        dump_representation(session, kind);
        ++actions;
    } else if (command == "dump_btor2_high_unopt" ||
               command == "-dump_btor2_high_unopt" ||
               command == "dump_btor2_high_opt" ||
               command == "-dump_btor2_high_opt" ||
               command == "dump_btor2_low_unopt" ||
               command == "-dump_btor2_low_unopt" ||
               command == "dump_btor2_low_opt" ||
               command == "-dump_btor2_low_opt") {
        require_count(words, 1);
        auto kind = command;
        if (kind.front() == '-') kind.erase(0, 1);
        kind.erase(0, 5);
        dump_representation(session, kind);
        ++actions;
    } else if (command == "dump_asm" || command == "-dump_asm") {
        require_count(words, 2);
        dump_representation(session, "asm", words[1]);
        ++actions;
    } else if (command == "prove" || command == "cover") {
        require_count(words, 2);
        const auto action = command == "prove" ? FormalAction::prove
                                                : FormalAction::cover;
        session.formal_property = words[1];
        ++actions;
        if (session.no_run)
            return;
        if (!run_formal(session, action, words[1]))
            throw std::runtime_error(command + " failed");
    } else {
        throw std::runtime_error("unknown command: " + command);
    }
}

void run_script(const std::filesystem::path& path, bool no_run,
                std::string_view command = {}) {
    const auto script = std::filesystem::absolute(path).lexically_normal();
    std::ifstream input(script);
    if (!input)
        throw std::runtime_error("cannot read script: " + script.string());
    Session session;
    session.no_run = no_run;
    session.formal_work_directory = script.parent_path() / "formal-work";
    std::string line;
    size_t line_number = 0;
    size_t actions = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const auto first_line = line_number;
        while (!line.empty() && line.back() == '\\') {
            line.pop_back();
            std::string continuation;
            if (!std::getline(input, continuation))
                throw std::runtime_error(
                    script.string() + ':' + std::to_string(first_line) +
                    ": unfinished continuation");
            ++line_number;
            line += ' ' + continuation;
        }
        try {
            execute_command(
                session, script.parent_path(), split_words(line), actions);
        } catch (const std::exception& error) {
            throw std::runtime_error(
                script.string() + ':' + std::to_string(first_line) + ": " +
                error.what());
        }
    }
    if (!command.empty()) {
        try {
            execute_command(session, script.parent_path(), split_words(command), actions);
        } catch (const std::exception& error) {
            throw std::runtime_error("command line: " + std::string(error.what()));
        }
    }
    if (!actions)
        throw std::runtime_error(script.string() + ": no run command");
}
}
}

int main(int argc, char** argv) {
    try {
        if (argc == 2) {
            const std::string option = argv[1];
            if (option == "-help" || option == "--help" || option == "-h") {
                emul::tool::print_help(std::cout);
                return 0;
            }
        }
        if ((argc < 3 || argc > 5) || std::string(argv[1]) != "-f") {
            emul::tool::print_help(std::cerr);
            return 1;
        }
        bool no_run = false;
        std::string_view command;
        if (argc >= 4) {
            if (std::string_view(argv[3]) == "-norun") {
                no_run = true;
                if (argc == 5) command = argv[4];
            } else if (argc == 4) {
                command = argv[3];
            } else {
                emul::tool::print_help(std::cerr);
                return 1;
            }
        }
        emul::tool::run_script(argv[2], no_run, command);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "hwc: " << error.what() << '\n';
        return 1;
    }
}
