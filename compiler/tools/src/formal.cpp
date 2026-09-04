/*
 * compiler/tools/src/formal.cpp
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

#include "formal.h"
#include "input.h"
#include "frontend/frontend.h"
#include "formal/engine/engine.h"
#include "formal/router/decision.h"
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iostream>

namespace emul::tool {
namespace {
std::filesystem::path make_work_directory(const Session& session,
                                           std::string_view property) {
    std::string name(property);
    for (auto& character : name)
        if (!std::isalnum(static_cast<unsigned char>(character)) &&
            character != '-' && character != '_') character = '_';
    if (name.empty()) name = "all";
    const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    auto path = session.formal_work_directory / name /
                ("run-" + std::to_string(stamp));
    for (unsigned suffix = 1; std::filesystem::exists(path); ++suffix)
        path = session.formal_work_directory / name /
               ("run-" + std::to_string(stamp) + '-' + std::to_string(suffix));
    std::filesystem::create_directories(path);
    return std::filesystem::absolute(path).lexically_normal();
}

bool expected(const Session& session, FormalAction action, formal::Result result) {
    const auto& value = session.expected_result;
    if (value == "any") return true;
    if (value == "proved") return result == formal::Result::proved;
    if (value == "bounded") return result == formal::Result::bounded;
    if (value == "covered") return result == formal::Result::covered;
    if (value == "counterexample") return result == formal::Result::counterexample;
    if (value == "unknown") return result == formal::Result::unknown;
    if (value == "timeout") return result == formal::Result::timeout;
    return action == FormalAction::cover ? result == formal::Result::covered :
        result == formal::Result::proved || result == formal::Result::bounded;
}

std::string vcd_timescale(std::string value) {
    if (value.empty()) return "1ns";
    if (const auto slash = value.find('/'); slash != std::string::npos)
        value.erase(slash);
    return value;
}
}

bool run_formal(
    const Session& session,
    FormalAction action,
    std::string_view property) {
    auto design = frontend::Frontend().elaborate(frontend_input(session));
    if (std::getenv("HWC_TRACE_FRONTEND"))
        design.print(std::cerr);
    formal::EngineOptions options;
    options.backend = formal::parse_backend(session.formal_backend);
    options.timeout = std::chrono::milliseconds(session.formal_timeout_ms);
    options.work_directory = make_work_directory(session, property);
    options.timescale = vcd_timescale(session.timescale);
    options.expected_result = session.expected_result;
    options.shortest_cover = session.cover_trace == "shortest";
    std::cout << "[formal] work directory: " << options.work_directory << '\n';
    auto decision = formal::router::select(
        design, session.top, property, session.formal_task,
        session.engine, session.max_depth);
    std::cout << "[formal] task: " << formal::router::name(decision.task)
              << '\n';
    std::cout << "[formal] route: " << formal::router::name(decision.engine)
              << " (" << decision.reason << ")\n";
    options.kind = formal::parse_engine(
        formal::router::name(decision.engine));
    options.depth = session.max_depth;
    options.pono = HWC_DEFAULT_PONO;
    options.btormc = HWC_DEFAULT_BTORMC;
    options.ric3 = HWC_DEFAULT_RIC3;
    options.avr = HWC_DEFAULT_AVR;
    options.abc = HWC_DEFAULT_ABC;
    options.avy = HWC_DEFAULT_AVY;
    options.suprove = HWC_DEFAULT_SUPROVE;
    options.btor2aiger = HWC_DEFAULT_BTOR2AIGER;
    options.yosys = HWC_DEFAULT_YOSYS;
    options.smtbmc = HWC_DEFAULT_SMTBMC;
    options.clock = session.clock;
    options.clocks = session.clocks;
    options.reset = session.reset;
    auto report = formal::run(
        design, session.top, property, options, std::cout);
    return expected(session, action, report.result);
}

}
