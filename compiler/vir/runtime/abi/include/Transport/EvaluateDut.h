#pragma once

#include "../Core/Result.h"
#include "runtime/transport/transport.h"
#include <string>

namespace vir::runtime {

class EvaluateDut {
public:
    EvaluateDut(transport::Transport& transport, uint64_t id,
                std::string instance);
    transport::Status execute();
    const transport::Job& job() const noexcept { return job_; }
private:
    transport::Transport& transport_;
    std::string instance_;
    transport::Job job_;
};

} // namespace vir::runtime
