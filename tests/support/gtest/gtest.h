#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace testing {

class Test {
  public:
    virtual ~Test() = default;

  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

namespace detail {

class AbortCurrentTest : public std::exception {
  public:
    const char* what() const noexcept override { return "fatal assertion failed"; }
};

struct TestResult {
    int assertionFailures = 0;
};

inline thread_local TestResult* g_currentResult = nullptr;

struct TestCase {
    std::string suiteName;
    std::string testName;
    std::function<void()> run;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failedTestCount() {
    static int count = 0;
    return count;
}

inline int& totalTestCount() {
    static int count = 0;
    return count;
}

inline void appendMessage(std::ostringstream&) {}

template <typename T, typename... Rest>
inline void appendMessage(std::ostringstream& stream, T&& value, Rest&&... rest) {
    stream << std::forward<T>(value);
    appendMessage(stream, std::forward<Rest>(rest)...);
}

template <typename... Args>
inline void recordFailure(const char* file, int line, bool fatal, Args&&... args) {
    if (g_currentResult != nullptr) {
        ++g_currentResult->assertionFailures;
    }

    std::ostringstream stream;
    appendMessage(stream, std::forward<Args>(args)...);
    stream << '\n';
    std::cerr << file << ":" << line << ": " << (fatal ? "fatal" : "failure") << ": "
              << stream.str();

    if (fatal) {
        throw AbortCurrentTest();
    }
}

class AssertionBuilder {
  public:
    AssertionBuilder(bool passed, bool fatal, const char* file, int line)
        : m_passed(passed), m_fatal(fatal), m_file(file), m_line(line) {}

    AssertionBuilder(const AssertionBuilder&) = delete;
    AssertionBuilder& operator=(const AssertionBuilder&) = delete;
    AssertionBuilder(AssertionBuilder&&) = default;
    AssertionBuilder& operator=(AssertionBuilder&&) = default;

    ~AssertionBuilder() noexcept(false) {
        if (!m_passed) {
            recordFailure(m_file, m_line, m_fatal, m_message.str());
        }
    }

    template <typename T>
    AssertionBuilder& operator<<(T&& value) {
        if (!m_passed) {
            m_message << std::forward<T>(value);
        }
        return *this;
    }

  private:
    bool m_passed;
    bool m_fatal;
    const char* m_file;
    int m_line;
    std::ostringstream m_message;
};

template <typename Lhs, typename Rhs>
inline AssertionBuilder compareEqual(const Lhs& lhs,
                                     const Rhs& rhs,
                                     bool fatal,
                                     const char* file,
                                     int line,
                                     const char* lhsText,
                                     const char* rhsText) {
    const bool passed = lhs == rhs;
    AssertionBuilder builder(passed, fatal, file, line);
    if (!passed) {
        builder << "expected equality: " << lhsText << " == " << rhsText;
    }
    return builder;
}

template <typename Lhs, typename Rhs>
inline AssertionBuilder compareNotEqual(const Lhs& lhs,
                                        const Rhs& rhs,
                                        bool fatal,
                                        const char* file,
                                        int line,
                                        const char* lhsText,
                                        const char* rhsText) {
    const bool passed = lhs != rhs;
    AssertionBuilder builder(passed, fatal, file, line);
    if (!passed) {
        builder << "expected inequality: " << lhsText << " != " << rhsText;
    }
    return builder;
}

inline AssertionBuilder compareStringEqual(const char* lhs,
                                           const char* rhs,
                                           bool fatal,
                                           const char* file,
                                           int line,
                                           const char* lhsText,
                                           const char* rhsText) {
    const bool passed =
        (lhs == nullptr && rhs == nullptr) ||
        (lhs != nullptr && rhs != nullptr && std::string(lhs) == std::string(rhs));
    AssertionBuilder builder(passed, fatal, file, line);
    if (!passed) {
        builder << "expected string equality: " << lhsText << " == " << rhsText;
    }
    return builder;
}

inline AssertionBuilder compareFloatEqual(float lhs,
                                          float rhs,
                                          bool fatal,
                                          const char* file,
                                          int line,
                                          const char* lhsText,
                                          const char* rhsText) {
    const float diff = std::fabs(lhs - rhs);
    const float lhsAbs = std::fabs(lhs);
    const float rhsAbs = std::fabs(rhs);
    const float scale = lhsAbs > rhsAbs ? lhsAbs : rhsAbs;
    const float tolerance =
        std::numeric_limits<float>::epsilon() * (scale > 1.0f ? scale : 1.0f) * 4.0f;
    const bool passed = diff <= tolerance;
    AssertionBuilder builder(passed, fatal, file, line);
    if (!passed) {
        builder << "expected float equality: " << lhsText << " ~= " << rhsText
                << " (" << std::setprecision(9) << lhs << " vs " << rhs << ")";
    }
    return builder;
}

inline AssertionBuilder expectTrue(bool condition,
                                   bool fatal,
                                   const char* file,
                                   int line,
                                   const char* expression) {
    AssertionBuilder builder(condition, fatal, file, line);
    if (!condition) {
        builder << "expected true: " << expression;
    }
    return builder;
}

inline AssertionBuilder expectFalse(bool condition,
                                    bool fatal,
                                    const char* file,
                                    int line,
                                    const char* expression) {
    AssertionBuilder builder(!condition, fatal, file, line);
    if (condition) {
        builder << "expected false: " << expression;
    }
    return builder;
}

inline bool registerTest(std::string suiteName, std::string testName, std::function<void()> run) {
    registry().push_back(TestCase{std::move(suiteName), std::move(testName), std::move(run)});
    return true;
}

inline int runAllTests() {
    int failedTests = 0;

    for (const TestCase& testCase : registry()) {
        ++totalTestCount();
        TestResult result;
        g_currentResult = &result;

        try {
            testCase.run();
        } catch (const AbortCurrentTest&) {
        } catch (const std::exception& ex) {
            recordFailure(__FILE__, __LINE__, false, "unexpected exception: ", ex.what());
        } catch (...) {
            recordFailure(__FILE__, __LINE__, false, "unexpected non-standard exception");
        }

        g_currentResult = nullptr;

        if (result.assertionFailures > 0) {
            ++failedTests;
        }
    }

    failedTestCount() = failedTests;
    return failedTests;
}

} // namespace detail

inline int InitGoogleTest(int*, char**) { return 0; }

inline int RUN_ALL_TESTS() { return detail::runAllTests(); }

} // namespace testing

#define TEST(test_suite_name, test_name)                                                           \
    class test_suite_name##_##test_name##_Test : public ::testing::Test {                          \
      public:                                                                                      \
        void TestBody();                                                                           \
        void Run() {                                                                               \
            SetUp();                                                                               \
            try {                                                                                  \
                TestBody();                                                                        \
            } catch (...) {                                                                        \
                TearDown();                                                                        \
                throw;                                                                             \
            }                                                                                      \
            TearDown();                                                                            \
        }                                                                                          \
    };                                                                                             \
    static const bool test_suite_name##_##test_name##_registered =                                 \
        ::testing::detail::registerTest(#test_suite_name, #test_name, []() {                       \
            test_suite_name##_##test_name##_Test testInstance;                                     \
            testInstance.Run();                                                                    \
        });                                                                                        \
    void test_suite_name##_##test_name##_Test::TestBody()

#define TEST_F(test_fixture, test_name)                                                            \
    class test_fixture##_##test_name##_Test : public test_fixture {                                \
      public:                                                                                      \
        void TestBody();                                                                           \
        void Run() {                                                                               \
            SetUp();                                                                               \
            try {                                                                                  \
                TestBody();                                                                        \
            } catch (...) {                                                                        \
                TearDown();                                                                        \
                throw;                                                                             \
            }                                                                                      \
            TearDown();                                                                            \
        }                                                                                          \
    };                                                                                             \
    static const bool test_fixture##_##test_name##_registered =                                    \
        ::testing::detail::registerTest(#test_fixture, #test_name, []() {                          \
            test_fixture##_##test_name##_Test testInstance;                                        \
            testInstance.Run();                                                                    \
        });                                                                                        \
    void test_fixture##_##test_name##_Test::TestBody()

#define EXPECT_TRUE(condition)                                                                      \
    ::testing::detail::expectTrue(static_cast<bool>(condition), false, __FILE__, __LINE__, #condition)
#define ASSERT_TRUE(condition)                                                                      \
    ::testing::detail::expectTrue(static_cast<bool>(condition), true, __FILE__, __LINE__, #condition)

#define EXPECT_FALSE(condition)                                                                     \
    ::testing::detail::expectFalse(static_cast<bool>(condition), false, __FILE__, __LINE__, #condition)
#define ASSERT_FALSE(condition)                                                                     \
    ::testing::detail::expectFalse(static_cast<bool>(condition), true, __FILE__, __LINE__, #condition)

#define EXPECT_EQ(lhs, rhs)                                                                         \
    ::testing::detail::compareEqual((lhs), (rhs), false, __FILE__, __LINE__, #lhs, #rhs)
#define ASSERT_EQ(lhs, rhs)                                                                         \
    ::testing::detail::compareEqual((lhs), (rhs), true, __FILE__, __LINE__, #lhs, #rhs)

#define EXPECT_NE(lhs, rhs)                                                                         \
    ::testing::detail::compareNotEqual((lhs), (rhs), false, __FILE__, __LINE__, #lhs, #rhs)
#define ASSERT_NE(lhs, rhs)                                                                         \
    ::testing::detail::compareNotEqual((lhs), (rhs), true, __FILE__, __LINE__, #lhs, #rhs)

#define EXPECT_STREQ(lhs, rhs)                                                                      \
    ::testing::detail::compareStringEqual((lhs), (rhs), false, __FILE__, __LINE__, #lhs, #rhs)
#define ASSERT_STREQ(lhs, rhs)                                                                      \
    ::testing::detail::compareStringEqual((lhs), (rhs), true, __FILE__, __LINE__, #lhs, #rhs)

#define EXPECT_FLOAT_EQ(lhs, rhs)                                                                   \
    ::testing::detail::compareFloatEqual((lhs), (rhs), false, __FILE__, __LINE__, #lhs, #rhs)
#define ASSERT_FLOAT_EQ(lhs, rhs)                                                                   \
    ::testing::detail::compareFloatEqual((lhs), (rhs), true, __FILE__, __LINE__, #lhs, #rhs)

#define SUCCEED() do { } while (false)
