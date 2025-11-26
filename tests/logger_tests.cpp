#include "hotplugpp/logger.hpp"

#include <gtest/gtest.h>

namespace hotplugpp {
namespace tests {

class LoggerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Reset logger to a known state
        Logger::instance().setLevel(LogLevel::Debug);
    }
};

// ============================================================================
// Logger Singleton Tests
// ============================================================================

TEST_F(LoggerTest, SingletonReturnsSameInstance) {
    Logger& instance1 = Logger::instance();
    Logger& instance2 = Logger::instance();
    EXPECT_EQ(&instance1, &instance2);
}

// ============================================================================
// Log Level Tests
// ============================================================================

TEST_F(LoggerTest, SetLevelTrace) {
    Logger::instance().setLevel(LogLevel::Trace);
    EXPECT_EQ(Logger::instance().getLevel(), LogLevel::Trace);
}

TEST_F(LoggerTest, SetLevelDebug) {
    Logger::instance().setLevel(LogLevel::Debug);
    EXPECT_EQ(Logger::instance().getLevel(), LogLevel::Debug);
}

TEST_F(LoggerTest, SetLevelInfo) {
    Logger::instance().setLevel(LogLevel::Info);
    EXPECT_EQ(Logger::instance().getLevel(), LogLevel::Info);
}

TEST_F(LoggerTest, SetLevelWarn) {
    Logger::instance().setLevel(LogLevel::Warn);
    EXPECT_EQ(Logger::instance().getLevel(), LogLevel::Warn);
}

TEST_F(LoggerTest, SetLevelError) {
    Logger::instance().setLevel(LogLevel::Error);
    EXPECT_EQ(Logger::instance().getLevel(), LogLevel::Error);
}

TEST_F(LoggerTest, SetLevelCritical) {
    Logger::instance().setLevel(LogLevel::Critical);
    EXPECT_EQ(Logger::instance().getLevel(), LogLevel::Critical);
}

TEST_F(LoggerTest, SetLevelOff) {
    Logger::instance().setLevel(LogLevel::Off);
    EXPECT_EQ(Logger::instance().getLevel(), LogLevel::Off);
}

// ============================================================================
// Init Tests
// ============================================================================

TEST_F(LoggerTest, InitSetsLevel) {
    Logger::instance().init(LogLevel::Warn);
    EXPECT_EQ(Logger::instance().getLevel(), LogLevel::Warn);
}

TEST_F(LoggerTest, InitWithPattern) {
    // Should not throw
    EXPECT_NO_THROW(Logger::instance().init(LogLevel::Info, "[%l] %v"));
}

// ============================================================================
// Logging Tests (No Exceptions)
// ============================================================================

TEST_F(LoggerTest, TraceDoesNotThrow) {
    EXPECT_NO_THROW(Logger::instance().trace("Test trace message"));
}

TEST_F(LoggerTest, DebugDoesNotThrow) {
    EXPECT_NO_THROW(Logger::instance().debug("Test debug message"));
}

TEST_F(LoggerTest, InfoDoesNotThrow) {
    EXPECT_NO_THROW(Logger::instance().info("Test info message"));
}

TEST_F(LoggerTest, WarnDoesNotThrow) {
    EXPECT_NO_THROW(Logger::instance().warn("Test warn message"));
}

TEST_F(LoggerTest, ErrorDoesNotThrow) {
    EXPECT_NO_THROW(Logger::instance().error("Test error message"));
}

TEST_F(LoggerTest, CriticalDoesNotThrow) {
    EXPECT_NO_THROW(Logger::instance().critical("Test critical message"));
}

// ============================================================================
// Macro Tests (No Exceptions)
// ============================================================================

TEST_F(LoggerTest, MacroTraceDoesNotThrow) {
    EXPECT_NO_THROW(HOTPLUGPP_LOG_TRACE("Test trace macro"));
}

TEST_F(LoggerTest, MacroDebugDoesNotThrow) {
    EXPECT_NO_THROW(HOTPLUGPP_LOG_DEBUG("Test debug macro"));
}

TEST_F(LoggerTest, MacroInfoDoesNotThrow) {
    EXPECT_NO_THROW(HOTPLUGPP_LOG_INFO("Test info macro"));
}

TEST_F(LoggerTest, MacroWarnDoesNotThrow) {
    EXPECT_NO_THROW(HOTPLUGPP_LOG_WARN("Test warn macro"));
}

TEST_F(LoggerTest, MacroErrorDoesNotThrow) {
    EXPECT_NO_THROW(HOTPLUGPP_LOG_ERROR("Test error macro"));
}

TEST_F(LoggerTest, MacroCriticalDoesNotThrow) {
    EXPECT_NO_THROW(HOTPLUGPP_LOG_CRITICAL("Test critical macro"));
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(LoggerTest, EmptyMessage) {
    EXPECT_NO_THROW(Logger::instance().info(""));
}

TEST_F(LoggerTest, LongMessage) {
    constexpr size_t kLongMessageLength = 10000;
    std::string longMsg(kLongMessageLength, 'x');
    EXPECT_NO_THROW(Logger::instance().info(longMsg));
}

TEST_F(LoggerTest, MessageWithSpecialCharacters) {
    EXPECT_NO_THROW(Logger::instance().info("Message with special chars: \n\t\r"));
}

TEST_F(LoggerTest, ChangeLevelMultipleTimes) {
    Logger::instance().setLevel(LogLevel::Trace);
    Logger::instance().setLevel(LogLevel::Debug);
    Logger::instance().setLevel(LogLevel::Info);
    Logger::instance().setLevel(LogLevel::Warn);
    Logger::instance().setLevel(LogLevel::Error);
    Logger::instance().setLevel(LogLevel::Critical);
    Logger::instance().setLevel(LogLevel::Off);
    EXPECT_EQ(Logger::instance().getLevel(), LogLevel::Off);
}

} // namespace tests
} // namespace hotplugpp
