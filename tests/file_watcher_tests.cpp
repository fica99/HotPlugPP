#include "hotplugpp/file_watcher.hpp"

#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>

class FileWatcherTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a test file
        testFilePath = std::string(TEST_PLUGIN_DIR) + "/test_watch_file.txt";
        std::ofstream ofs(testFilePath);
        ofs << "initial content";
        ofs.close();
    }

    void TearDown() override {
        // Remove test file
        std::remove(testFilePath.c_str());
    }

    std::string testFilePath;
};

TEST_F(FileWatcherTest, InitialState) {
    hotplugpp::FileWatcher watcher;
    EXPECT_FALSE(watcher.isRunning());
    EXPECT_FALSE(watcher.isWatching(testFilePath));
}

TEST_F(FileWatcherTest, StartAndStop) {
    hotplugpp::FileWatcher watcher;

    watcher.start();
    EXPECT_TRUE(watcher.isRunning());

    watcher.stop();
    EXPECT_FALSE(watcher.isRunning());
}

TEST_F(FileWatcherTest, WatchFile) {
    hotplugpp::FileWatcher watcher;

    bool callbackInvoked = false;
    auto callback = [&callbackInvoked](const std::string&) { callbackInvoked = true; };

    EXPECT_TRUE(watcher.watchFile(testFilePath, callback));
    EXPECT_TRUE(watcher.isWatching(testFilePath));
}

TEST_F(FileWatcherTest, WatchEmptyPath) {
    hotplugpp::FileWatcher watcher;

    bool callbackInvoked = false;
    auto callback = [&callbackInvoked](const std::string&) { callbackInvoked = true; };

    EXPECT_FALSE(watcher.watchFile("", callback));
}

TEST_F(FileWatcherTest, WatchWithNullCallback) {
    hotplugpp::FileWatcher watcher;

    EXPECT_FALSE(watcher.watchFile(testFilePath, nullptr));
}

TEST_F(FileWatcherTest, UnwatchFile) {
    hotplugpp::FileWatcher watcher;

    bool callbackInvoked = false;
    auto callback = [&callbackInvoked](const std::string&) { callbackInvoked = true; };

    watcher.watchFile(testFilePath, callback);
    EXPECT_TRUE(watcher.isWatching(testFilePath));

    watcher.unwatchFile(testFilePath);
    EXPECT_FALSE(watcher.isWatching(testFilePath));
}

TEST_F(FileWatcherTest, UnwatchNonExistentFile) {
    hotplugpp::FileWatcher watcher;

    // Should not crash when unwatching a file that was never watched
    watcher.unwatchFile("/nonexistent/path/file.txt");
}

TEST_F(FileWatcherTest, WatchMultipleFiles) {
    std::string testFilePath2 = std::string(TEST_PLUGIN_DIR) + "/test_watch_file2.txt";
    std::ofstream ofs(testFilePath2);
    ofs << "content";
    ofs.close();

    hotplugpp::FileWatcher watcher;

    auto callback = [](const std::string&) {};

    EXPECT_TRUE(watcher.watchFile(testFilePath, callback));
    EXPECT_TRUE(watcher.watchFile(testFilePath2, callback));

    EXPECT_TRUE(watcher.isWatching(testFilePath));
    EXPECT_TRUE(watcher.isWatching(testFilePath2));

    watcher.unwatchFile(testFilePath);
    EXPECT_FALSE(watcher.isWatching(testFilePath));
    EXPECT_TRUE(watcher.isWatching(testFilePath2));

    std::remove(testFilePath2.c_str());
}

TEST_F(FileWatcherTest, StartStopMultipleTimes) {
    hotplugpp::FileWatcher watcher;

    for (int i = 0; i < 3; ++i) {
        watcher.start();
        EXPECT_TRUE(watcher.isRunning());

        watcher.stop();
        EXPECT_FALSE(watcher.isRunning());
    }
}

TEST_F(FileWatcherTest, DoubleStart) {
    hotplugpp::FileWatcher watcher;

    watcher.start();
    EXPECT_TRUE(watcher.isRunning());

    // Second start should be a no-op
    watcher.start();
    EXPECT_TRUE(watcher.isRunning());

    watcher.stop();
    EXPECT_FALSE(watcher.isRunning());
}

TEST_F(FileWatcherTest, FileChangeDetection) {
    hotplugpp::FileWatcher watcher;

    std::atomic<bool> callbackInvoked{false};
    // Must watch BEFORE starting the watcher to ensure inotify watch is registered
    ASSERT_TRUE(watcher.watchFile(testFilePath, [&callbackInvoked](const std::string&) {
        callbackInvoked.store(true);
    }));

    watcher.start();

    // Wait for the watcher to fully initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Modify file
    {
        std::ofstream ofs(testFilePath, std::ios::out | std::ios::trunc);
        ofs << "modified content at " << std::chrono::system_clock::now().time_since_epoch().count();
        ofs.flush();
        ofs.close();
    }

    // Wait for notification (file system events may have some delay, especially in CI)
    for (int i = 0; i < 30 && !callbackInvoked.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Note: This test may be flaky in some CI environments due to timing issues
    // with file system notifications. The core functionality is also tested
    // indirectly through other integration tests.
    EXPECT_TRUE(callbackInvoked.load()) << "File change notification was not received within timeout";
}
