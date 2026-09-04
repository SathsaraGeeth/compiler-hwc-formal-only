/* Writes structured progress messages for formal runs. */

#pragma once
#include <iosfwd>
#include <string_view>

namespace emul::formal {
class Logger {
public:
    explicit Logger(std::ostream& output);
    void phase(std::string_view name, std::string_view detail = {});
    void result(std::string_view status, std::string_view detail = {});
private:
    std::ostream& output_;
};
}
