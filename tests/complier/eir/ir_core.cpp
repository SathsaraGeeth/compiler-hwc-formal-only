/* Exercises the public EIR construction, query, and verification contract. */

#include "eir/include/IR/Builder.h"
#include "eir/include/IR/Parser.h"
#include "eir/include/IR/Printer.h"
#include "eir/include/IR/Verifier.h"
#include <cassert>
#include <sstream>

int main() {
    using namespace emul::eir;

    auto type = Type::parse("4s<8>");
    assert(type.width() == 8);
    assert(type.four_state());

    Module module{.name = "ir_core"};
    Builder builder(module);
    builder.add_input("%a", type.spelling);
    builder.add_result("sum", type.spelling);
    builder.add_state("@q", type.spelling);
    auto& add = builder.create("add", "%a, 4s<8>'0x01", type.spelling);
    auto sum = add.result;
    builder.create("state_write", "@q, " + sum);
    builder.create("yield", sum);

    Program program;
    program.modules.push_back(std::move(module));
    verify(program);

    assert(program.root() != nullptr);
    assert(program.find_module("ir_core") == program.root());
    assert(program.root()->find_input("%a") != nullptr);
    assert(program.root()->find_state("@q") != nullptr);
    assert(program.root()->operations.front().operand_list().size() == 2);

    std::stringstream assembly;
    print(program, assembly);
    auto reparsed = parse(assembly);
    verify(reparsed);
    assert(reparsed.root()->name == "ir_core");
    assert(reparsed.root()->results.front().name == "sum");
}
