#include "decision.h"

namespace emul::formal::router {

Decision select(const frontend::ElaboratedDesign& design, std::string_view top,
                std::string_view property, std::string_view task_override,
                std::string_view engine_override, std::uint32_t bound) {
    return route({btor2::profile(design, top, property, bound),
                  parse_task(task_override), parse_engine(engine_override)});
}

} // namespace emul::formal::router
