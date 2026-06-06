module;
#include <chrono>
#include <span>
#include <string>
#include <utility>

module engine.log;

Log::Log() : start_(std::chrono::steady_clock::now()) {
    entries_.reserve(CAPACITY);
}

Log& Log::instance() {
    static Log inst;
    return inst;
}

void Log::push(LogLevel level, std::string message) {
    if (entries_.size() >= CAPACITY)
        entries_.erase(entries_.begin());

    const auto now = std::chrono::steady_clock::now();
    const float t = std::chrono::duration<float>(now - start_).count();
    entries_.push_back(LogEntry{ level, t, std::move(message) });
}

std::span<const LogEntry> Log::entries() const {
    return { entries_.data(), entries_.size() };
}

void Log::clear() {
    entries_.clear();
}

void log_trace(std::string msg) { Log::instance().push(LogLevel::Trace, std::move(msg)); }
void log_info (std::string msg) { Log::instance().push(LogLevel::Info,  std::move(msg)); }
void log_warn (std::string msg) { Log::instance().push(LogLevel::Warn,  std::move(msg)); }
void log_error(std::string msg) { Log::instance().push(LogLevel::Error, std::move(msg)); }
