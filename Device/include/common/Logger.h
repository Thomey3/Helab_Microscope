#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include <fstream>
#include <mutex>
#include <memory>
#include <format>
#include <source_location>

namespace helab {

/// Log level enumeration
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

/// Convert log level to string
constexpr std::string_view levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT";
        default:                 return "UNKNOWN";
    }
}

/// Thread-safe logger with file and console output
class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLogFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.close();
        }
        file_.open(path, std::ios::app);
    }

    void setLevel(LogLevel level) {
        level_ = level;
    }

    LogLevel level() const {
        return level_;
    }

    void log(LogLevel level, std::string_view message,
             const std::source_location& loc = std::source_location::current()) {
        if (level < level_) return;

        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::format("%Y-%m-%d %H:%M:%S", now);

        std::string logLine = std::format("[{}] {} {}:{}: {}",
            timestamp,
            levelToString(level),
            loc.file_name(),
            loc.line(),
            message
        );

        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_ << logLine << '\n';
            file_.flush();
        }
    }

    void trace(std::string_view msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Trace, msg, loc);
    }

    void debug(std::string_view msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Debug, msg, loc);
    }

    void info(std::string_view msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Info, msg, loc);
    }

    void warning(std::string_view msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Warning, msg, loc);
    }

    void error(std::string_view msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Error, msg, loc);
    }

    void critical(std::string_view msg, const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Critical, msg, loc);
    }

private:
    Logger() = default;
    ~Logger() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex mutex_;
    std::ofstream file_;
    LogLevel level_ = LogLevel::Info;
};

/// Convenience macros for logging
#define LOG_TRACE(msg) helab::Logger::instance().trace(msg)
#define LOG_DEBUG(msg) helab::Logger::instance().debug(msg)
#define LOG_INFO(msg)  helab::Logger::instance().info(msg)
#define LOG_WARN(msg)  helab::Logger::instance().warning(msg)
#define LOG_ERROR(msg) helab::Logger::instance().error(msg)
#define LOG_CRIT(msg)  helab::Logger::instance().critical(msg)

} // namespace helab
