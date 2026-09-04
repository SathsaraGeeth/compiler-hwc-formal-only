/*
 * compiler/frontend/lib/frontend.cpp
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

#include "frontend/frontend.h"
#include "frontend/semantic_tree.h"
#include "slang/ast/Compilation.h"
#include "slang/ast/ASTSerializer.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/driver/Driver.h"
#include "slang/text/Json.h"
#include <stdexcept>

namespace emul::frontend {
namespace {
void serialize_user_design(slang::ast::ASTSerializer& serializer,
                           const slang::ast::RootSymbol& root) {
    serializer.startObject();
    serializer.write("name", root.name);
    serializer.write("kind", toString(root.kind));
    serializer.write("addr", reinterpret_cast<uint64_t>(&root));
    serializer.startArray("members");
    for (const auto& member : root.members()) {
        if (member.kind == slang::ast::SymbolKind::CompilationUnit &&
            !member.as<slang::ast::CompilationUnitSymbol>()
                 .sourceLibrary.isDefault)
            continue;
        serializer.serialize(member, true);
    }
    serializer.endArray();
    serializer.endObject();
}
}

ElaboratedDesign Frontend::elaborate(const FrontendInput& input) const {
    slang::driver::Driver driver;
    driver.addStandardArgs();
    std::vector<std::string> args{
        "eirc-new",
        "--relax-enum-conversions"
    };
    if (!input.top.empty()) { args.emplace_back("--top"); args.push_back(input.top); }
    if (!input.timescale.empty()) {
        args.emplace_back("--timescale"); args.push_back(input.timescale);
    }
    for (const auto& dir : input.include_dirs) {
        args.emplace_back("-I"); args.push_back(dir.string());
    }
    for (const auto& define : input.defines) {
        args.emplace_back("-D"); args.push_back(define);
    }
    for (const auto& source : input.sources) args.push_back(source.string());
    for (const auto& source : input.library_sources) {
        args.emplace_back("-v"); args.push_back("eirc_lib=" + source.string());
    }
    std::vector<const char*> argv;
    for (const auto& arg : args) argv.push_back(arg.c_str());
    if (!driver.parseCommandLine(static_cast<int>(argv.size()), argv.data()) ||
        !driver.processOptions() || !driver.parseAllSources())
        throw std::runtime_error("Slang could not parse the inputs");
    auto compilation = driver.createCompilation();
    driver.reportCompilation(*compilation, false);
    if (compilation->hasIssuedErrors() || driver.diagEngine.getNumErrors() != 0)
        throw std::runtime_error("SystemVerilog compilation failed");
    slang::JsonWriter writer;
    slang::ast::ASTSerializer serializer(*compilation, writer);
    serializer.startObject();
    serializer.setIncludeAddresses(true);
    serializer.setIncludeSourceInfo(true);
    serializer.setDetailedTypeInfo(false);
    serializer.setTryConstantFold(true);
    serializer.writeProperty("design");
    serialize_user_design(serializer, compilation->getRoot());
    serializer.endObject();
    return ElaboratedDesign(SemanticTree::parse(writer.view()));
}
}  
