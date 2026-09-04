#pragma once
#include "VIR/BasicBlock.h"
#include <string>
#include <unordered_map>
#include <vector>
namespace vir::irgen
{
class SSABuilder
{
public:
void write(std::string name, BasicBlock &block, Value &value);
Value * read(const std::string &name, BasicBlock &block, Type type);
void add_predecessor(BasicBlock &block, BasicBlock &predecessor);
void seal(BasicBlock &block);
bool sealed(const BasicBlock &block) const;
private:
struct BlockState {
    bool sealed = false;
    std::vector<BasicBlock *> predecessors;
    std::unordered_map<std::string, Value *> values;
    std::unordered_map<std::string, Value *> incomplete;
};
std::unordered_map<BasicBlock *, BlockState> states_;
Value * read_recursive(const std::string &, BasicBlock &, Type);
};
}
