#include "Transport/EvaluateDut.h"

namespace vir::runtime {

EvaluateDut::EvaluateDut(transport::Transport& transport, uint64_t id,
                         std::string instance)
    : transport_(transport), instance_(std::move(instance)),
      job_{id, instance_.c_str()} {}

transport::Status EvaluateDut::execute() {
    return transport_.try_schedule_job(job_);
}

} // namespace vir::runtime
