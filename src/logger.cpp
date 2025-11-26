#include "hotplugpp/logger.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace hotplugpp {

namespace {
spdlog::level::level_enum toSpdlogLevel(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
        return spdlog::level::trace;
    case LogLevel::Debug:
        return spdlog::level::debug;
    case LogLevel::Info:
        return spdlog::level::info;
    case LogLevel::Warn:
        return spdlog::level::warn;
    case LogLevel::Error:
        return spdlog::level::err;
    case LogLevel::Critical:
        return spdlog::level::critical;
    case LogLevel::Off:
        return spdlog::level::off;
    default:
        return spdlog::level::info;
    }
}
} // namespace

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
#ifdef NDEBUG
    : m_level(LogLevel::Warn)
#else
    : m_level(LogLevel::Debug)
#endif
{
    // Create console logger with colors
    m_logger = spdlog::stdout_color_mt("hotplugpp");
    m_logger->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
    m_logger->set_level(toSpdlogLevel(m_level));
}

Logger::~Logger() {
    spdlog::drop("hotplugpp");
}

void Logger::init(LogLevel level, const std::string& pattern) {
    setLevel(level);
    if (!pattern.empty()) {
        m_logger->set_pattern(pattern);
    }
}

void Logger::setLevel(LogLevel level) {
    m_level = level;
    m_logger->set_level(toSpdlogLevel(level));
}

LogLevel Logger::getLevel() const {
    return m_level;
}

void Logger::trace(const std::string& message) {
    m_logger->trace(message);
}

void Logger::debug(const std::string& message) {
    m_logger->debug(message);
}

void Logger::info(const std::string& message) {
    m_logger->info(message);
}

void Logger::warn(const std::string& message) {
    m_logger->warn(message);
}

void Logger::error(const std::string& message) {
    m_logger->error(message);
}

void Logger::critical(const std::string& message) {
    m_logger->critical(message);
}

} // namespace hotplugpp
