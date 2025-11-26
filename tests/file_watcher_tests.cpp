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
