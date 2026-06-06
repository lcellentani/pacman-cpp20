module;
#include <chrono>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

export module engine.log;

export enum class LogLevel { Trace, Info, Warn, Error };

export struct LogEntry {
    LogLevel    level;
    float       time_s;       // seconds since first Log::instance() call
    std::string message;
};

export class Log {
public:
    static constexpr std::size_t CAPACITY = 2048;

    static Log& instance();

    void push(LogLevel level, std::string message);
    [[nodiscard]] std::span<const LogEntry> entries() const;
    void clear();

private:
    Log();

    // Linear vector with erase-front when full. Simple and gives entries()
    // a contiguous, in-order span without a second flat buffer.
    std::vector<LogEntry> entries_;
    std::chrono::steady_clock::time_point start_;
};

export void log_trace(std::string msg);
export void log_info (std::string msg);
export void log_warn (std::string msg);
export void log_error(std::string msg);
