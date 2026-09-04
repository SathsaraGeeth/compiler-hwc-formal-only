#pragma once
#include <functional>
#include <mutex>
#include <string_view>

namespace vir::runtime::builtin {
class Logger {
public:
    enum class Level { info, warning, error, fatal };
    using Sink = std::function<void(Level, std::string_view)>;
    explicit Logger(Sink sink = {});
    void log(Level level, std::string_view message) const;
    void info(std::string_view value) const { log(Level::info, value); }
    void warning(std::string_view value) const { log(Level::warning, value); }
    void error(std::string_view value) const { log(Level::error, value); }
    void fatal(std::string_view value) const { log(Level::fatal, value); }
    void set_level(Level level) { level_ = level; }
private:
    mutable std::mutex mutex_;
    Sink sink_;
    Level level_ = Level::info;
};
}
