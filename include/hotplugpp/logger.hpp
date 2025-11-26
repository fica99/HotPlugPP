#pragma once

#include <memory>
#include <string>

// Forward declaration for spdlog
namespace spdlog {
class logger;
}

namespace hotplugpp {

/**
 * @brief Log level enumeration
 */
enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical, Off };

/**
 * @brief Logger interface for HotPlugPP
 *
 * Provides a singleton logger that can be configured at runtime.
 * In Release builds, logging can be disabled at compile time for performance.
 */
class Logger {
  public:
    /**
     * @brief Get the singleton logger instance
     * @return Reference to the logger instance
     */
    static Logger& instance();

    /**
     * @brief Initialize the logger with a specific log level
     * @param level Minimum log level to output
     * @param pattern Optional log pattern (spdlog format)
     */
    void init(LogLevel level = LogLevel::Info, const std::string& pattern = "");

    /**
     * @brief Set the log level at runtime
     * @param level New minimum log level
     */
    void setLevel(LogLevel level);

    /**
     * @brief Get the current log level
     * @return Current log level
     */
    LogLevel getLevel() const;

    /**
     * @brief Log a trace message
     * @param message Message to log
     */
    void trace(const std::string& message);

    /**
     * @brief Log a debug message
     * @param message Message to log
     */
    void debug(const std::string& message);

    /**
     * @brief Log an info message
     * @param message Message to log
     */
    void info(const std::string& message);

    /**
     * @brief Log a warning message
     * @param message Message to log
     */
    void warn(const std::string& message);

    /**
     * @brief Log an error message
     * @param message Message to log
     */
    void error(const std::string& message);

    /**
     * @brief Log a critical message
     * @param message Message to log
     */
    void critical(const std::string& message);

  private:
    Logger();
    ~Logger();

    // Disable copy and move
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    std::shared_ptr<spdlog::logger> m_logger;
    LogLevel m_level;
};

// Convenience macros for logging
// In Release builds without HOTPLUGPP_ENABLE_LOGGING, these become no-ops
// Note: Logger::instance() uses Meyers' singleton pattern which is thread-safe
// and well-optimized by modern compilers. The singleton access overhead is
// negligible compared to actual logging operations.

#if defined(NDEBUG) && !defined(HOTPLUGPP_ENABLE_LOGGING)
// Release build without explicit logging enabled - no-ops for trace/debug/info
#define HOTPLUGPP_LOG_TRACE(msg) ((void)0)
#define HOTPLUGPP_LOG_DEBUG(msg) ((void)0)
#define HOTPLUGPP_LOG_INFO(msg) ((void)0)
#define HOTPLUGPP_LOG_WARN(msg) \
    do { \
        auto& logger_ = ::hotplugpp::Logger::instance(); \
        if (logger_.getLevel() <= ::hotplugpp::LogLevel::Warn) \
            logger_.warn(msg); \
    } while (0)
#define HOTPLUGPP_LOG_ERROR(msg) \
    do { \
        auto& logger_ = ::hotplugpp::Logger::instance(); \
        if (logger_.getLevel() <= ::hotplugpp::LogLevel::Error) \
            logger_.error(msg); \
    } while (0)
#define HOTPLUGPP_LOG_CRITICAL(msg) \
    do { \
        auto& logger_ = ::hotplugpp::Logger::instance(); \
        if (logger_.getLevel() <= ::hotplugpp::LogLevel::Critical) \
            logger_.critical(msg); \
    } while (0)
#else
// Debug build or explicit logging enabled
#define HOTPLUGPP_LOG_TRACE(msg) \
    do { \
        auto& logger_ = ::hotplugpp::Logger::instance(); \
        if (logger_.getLevel() <= ::hotplugpp::LogLevel::Trace) \
            logger_.trace(msg); \
    } while (0)
#define HOTPLUGPP_LOG_DEBUG(msg) \
    do { \
        auto& logger_ = ::hotplugpp::Logger::instance(); \
        if (logger_.getLevel() <= ::hotplugpp::LogLevel::Debug) \
            logger_.debug(msg); \
    } while (0)
#define HOTPLUGPP_LOG_INFO(msg) \
    do { \
        auto& logger_ = ::hotplugpp::Logger::instance(); \
        if (logger_.getLevel() <= ::hotplugpp::LogLevel::Info) \
            logger_.info(msg); \
    } while (0)
#define HOTPLUGPP_LOG_WARN(msg) \
    do { \
        auto& logger_ = ::hotplugpp::Logger::instance(); \
        if (logger_.getLevel() <= ::hotplugpp::LogLevel::Warn) \
            logger_.warn(msg); \
    } while (0)
#define HOTPLUGPP_LOG_ERROR(msg) \
    do { \
        auto& logger_ = ::hotplugpp::Logger::instance(); \
        if (logger_.getLevel() <= ::hotplugpp::LogLevel::Error) \
            logger_.error(msg); \
    } while (0)
#define HOTPLUGPP_LOG_CRITICAL(msg) \
    do { \
        auto& logger_ = ::hotplugpp::Logger::instance(); \
        if (logger_.getLevel() <= ::hotplugpp::LogLevel::Critical) \
            logger_.critical(msg); \
    } while (0)
#endif

} // namespace hotplugpp
