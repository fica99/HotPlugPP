#pragma once

#include <cmath>
#include <cstring>
#include <exception>
#include <iostream>
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

struct TestRecord {
    const char* suite;
    const char* name;
    void (*run)();
};

inline std::vector<TestRecord>& getRegistry() {
    static std::vector<TestRecord> registry;
    return registry;
}

inline bool RegisterTest(const char* suite, const char* name, void (*run)()) {
    getRegistry().push_back(TestRecord{suite, name, run});
    return true;
}

namespace internal {

class FatalFailure : public std::exception {
  public:
    const char* what() const noexcept override { return "fatal test failure"; }
};

class MessageSink {
  public:
    template <typename T>
    MessageSink& operator<<(const T&) {
        return *this;
    }
};

struct TestState {
    int failures = 0;
};

inline TestState*& activeTestState() {
    static TestState* state = nullptr;
    return state;
}

inline MessageSink reportFailure(const char* file, int line, const std::string& message, bool fatal) {
    std::cerr << file << ":" << line << ": Failure" << std::endl;
    std::cerr << message << std::endl;

    if (activeTestState() != nullptr) {
        ++activeTestState()->failures;
    }

    if (fatal) {
        throw FatalFailure();
    }

    return MessageSink{};
}

inline MessageSink succeed() { return MessageSink{}; }

inline MessageSink expectTrue(bool condition, const char* expression, const char* file, int line,
                              bool fatal) {
    if (condition) {
        return MessageSink{};
    }

    return reportFailure(file, line, std::string("Expected true: ") + expression, fatal);
}

inline MessageSink expectFalse(bool condition, const char* expression, const char* file, int line,
                               bool fatal) {
    if (!condition) {
        return MessageSink{};
    }

    return reportFailure(file, line, std::string("Expected false: ") + expression, fatal);
}

template <typename Lhs, typename Rhs>
inline MessageSink expectEqual(const Lhs& lhs, const Rhs& rhs, const char* lhsExpr,
                               const char* rhsExpr, const char* file, int line, bool fatal) {
    if (lhs == rhs) {
        return MessageSink{};
    }

    return reportFailure(file, line,
                         std::string("Expected equality: ") + lhsExpr + " == " + rhsExpr, fatal);
}

template <typename Lhs, typename Rhs>
inline MessageSink expectNotEqual(const Lhs& lhs, const Rhs& rhs, const char* lhsExpr,
                                  const char* rhsExpr, const char* file, int line, bool fatal) {
    if (lhs != rhs) {
        return MessageSink{};
    }

    return reportFailure(file, line,
                         std::string("Expected inequality: ") + lhsExpr + " != " + rhsExpr,
                         fatal);
}

inline MessageSink expectStringEqual(const char* lhs, const char* rhs, const char* lhsExpr,
                                     const char* rhsExpr, const char* file, int line, bool fatal) {
    const bool equal = lhs == rhs || (lhs != nullptr && rhs != nullptr && std::strcmp(lhs, rhs) == 0);
    if (equal) {
        return MessageSink{};
    }

    return reportFailure(file, line,
                         std::string("Expected strings to match: ") + lhsExpr + " vs " + rhsExpr,
                         fatal);
}

inline MessageSink expectFloatEqual(float lhs, float rhs, const char* lhsExpr, const char* rhsExpr,
                                    const char* file, int line, bool fatal) {
    const float difference = std::fabs(lhs - rhs);
    const float tolerance = 0.0001f;
    if (difference <= tolerance) {
        return MessageSink{};
    }

    return reportFailure(file, line,
                         std::string("Expected floats to match: ") + lhsExpr + " vs " + rhsExpr,
                         fatal);
}

} // namespace internal

inline void InitGoogleTest(int*, char**) {}

inline int RunAllTests() {
    int failedTests = 0;
    int executedTests = 0;

    for (const TestRecord& record : getRegistry()) {
        internal::TestState state;
        internal::activeTestState() = &state;
        ++executedTests;

        try {
            record.run();
        } catch (const internal::FatalFailure&) {
            // Failure already recorded by the assertion helper.
        } catch (const std::exception& ex) {
            internal::reportFailure(__FILE__, __LINE__,
                                    std::string("Unhandled exception: ") + ex.what(), false);
        } catch (...) {
            internal::reportFailure(__FILE__, __LINE__, "Unhandled non-standard exception", false);
        }

        if (state.failures == 0) {
            std::cout << "[  PASSED  ] " << record.suite << "." << record.name << std::endl;
        } else {
            ++failedTests;
            std::cout << "[  FAILED  ] " << record.suite << "." << record.name << std::endl;
        }
    }

    internal::activeTestState() = nullptr;
    std::cout << "Executed " << executedTests << " test(s)." << std::endl;
    return failedTests == 0 ? 0 : 1;
}

} // namespace testing

#define RUN_ALL_TESTS() ::testing::RunAllTests()

#define TEST(test_suite_name, test_name)                                                   \
    static void test_suite_name##_##test_name##_Test();                                    \
    namespace {                                                                            \
    const bool test_suite_name##_##test_name##_registered =                                \
        ::testing::RegisterTest(#test_suite_name, #test_name,                              \
                                &test_suite_name##_##test_name##_Test);                    \
    }                                                                                      \
    static void test_suite_name##_##test_name##_Test()

#define TEST_F(test_fixture, test_name)                                                    \
    class test_fixture##_##test_name##_Test : public test_fixture {                        \
      public:                                                                              \
        void TestBody();                                                                   \
        static void Run() {                                                                \
            test_fixture##_##test_name##_Test testInstance;                                \
            bool didSetUp = false;                                                         \
            try {                                                                          \
                testInstance.SetUp();                                                      \
                didSetUp = true;                                                           \
                testInstance.TestBody();                                                   \
            } catch (const ::testing::internal::FatalFailure&) {                           \
            }                                                                              \
            if (didSetUp) {                                                                \
                testInstance.TearDown();                                                   \
            }                                                                              \
        }                                                                                  \
    };                                                                                     \
    namespace {                                                                            \
    const bool test_fixture##_##test_name##_registered =                                   \
        ::testing::RegisterTest(#test_fixture, #test_name,                                 \
                                &test_fixture##_##test_name##_Test::Run);                  \
    }                                                                                      \
    void test_fixture##_##test_name##_Test::TestBody()

#define EXPECT_TRUE(condition)                                                             \
    ::testing::internal::expectTrue(static_cast<bool>(condition), #condition, __FILE__,    \
                                    __LINE__, false)

#define EXPECT_FALSE(condition)                                                            \
    ::testing::internal::expectFalse(static_cast<bool>(condition), #condition, __FILE__,   \
                                     __LINE__, false)

#define ASSERT_TRUE(condition)                                                             \
    ::testing::internal::expectTrue(static_cast<bool>(condition), #condition, __FILE__,    \
                                    __LINE__, true)

#define ASSERT_FALSE(condition)                                                            \
    ::testing::internal::expectFalse(static_cast<bool>(condition), #condition, __FILE__,   \
                                     __LINE__, true)

#define EXPECT_EQ(lhs, rhs)                                                                \
    ::testing::internal::expectEqual((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__, false)

#define EXPECT_NE(lhs, rhs)                                                                \
    ::testing::internal::expectNotEqual((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__,     \
                                        false)

#define ASSERT_EQ(lhs, rhs)                                                                \
    ::testing::internal::expectEqual((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__, true)

#define ASSERT_NE(lhs, rhs)                                                                \
    ::testing::internal::expectNotEqual((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__,     \
                                        true)

#define EXPECT_STREQ(lhs, rhs)                                                             \
    ::testing::internal::expectStringEqual((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__,  \
                                           false)

#define ASSERT_STREQ(lhs, rhs)                                                             \
    ::testing::internal::expectStringEqual((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__,  \
                                           true)

#define EXPECT_FLOAT_EQ(lhs, rhs)                                                          \
    ::testing::internal::expectFloatEqual((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__,   \
                                          false)

#define ASSERT_FLOAT_EQ(lhs, rhs)                                                          \
    ::testing::internal::expectFloatEqual((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__,   \
                                          true)

#define SUCCEED() ::testing::internal::succeed()
