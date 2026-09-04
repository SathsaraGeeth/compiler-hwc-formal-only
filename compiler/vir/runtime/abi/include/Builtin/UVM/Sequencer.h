#pragma once

#include "Builtin/Channel/Channel.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace vir::runtime::builtin::uvm {

class Sequencer {
public:
    void start_item(std::string sequencer, Value item);
    Value get_next_item(std::string_view sequencer);
    std::optional<Value> try_next_item(std::string_view sequencer);
    void reset();

private:
    std::shared_ptr<Channel> channel(std::string_view name);
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Channel>> channels_;
};

} // namespace vir::runtime::builtin::uvm
