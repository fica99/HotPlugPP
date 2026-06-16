#include "hotplugpp/plugin_loader.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <thread>

namespace hotplugpp {
namespace tests {

namespace fs = std::filesystem;

namespace {

std::string getDefinitelyMissingPluginPath() {
    return (fs::temp_directory_path() / "hotplugpp_missing_plugin" / "missing_plugin.bin").string();
}

std::string getInvalidPluginPath() {
    return (fs::temp_directory_path() / "hotplugpp_invalid_plugin.bin").string();
}

fs::path makePluginCopyForReloadTest(const std::string& sourcePath,
                                    const std::string& suffix = SHARED_LIB_SUFFIX) {
    const auto uniqueId =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path copyPath =
        fs::temp_directory_path() / ("hotplugpp_reload_test_" + uniqueId + suffix);

    std::error_code error;
    fs::copy_file(sourcePath, copyPath, fs::copy_options::overwrite_existing, error);
    if (error) {
        return {};
    }

    return copyPath;
}

bool touchPluginFile(const fs::path& path) {
    std::error_code error;
    const auto currentWriteTime = fs::last_write_time(path, error);
    if (error) {
        return false;
    }

    fs::last_write_time(path, currentWriteTime + std::chrono::seconds(1), error);
    return !error;
}

// Filename prefix the loader uses for its co-located shadow copies. Kept in sync
// with makeShadowCopy() in src/plugin_loader.cpp.
constexpr const char* kShadowPrefix = ".hotplugpp-shadow-";

// Creates a fresh, uniquely-named temp subdirectory so shadow-copy assertions are
// isolated from other tests, parallel runs, and stale files. Returns {} on error.
fs::path makeUniqueTempSubdir() {
    static std::atomic<unsigned long long> counter{0};
    std::random_device rd;
    const auto uniqueId =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_" +
        std::to_string(counter.fetch_add(1)) + "_" + std::to_string(rd());
    const fs::path dir = fs::temp_directory_path() / ("hotplugpp_test_" + uniqueId);

    std::error_code error;
    fs::create_directories(dir, error);
    if (error) {
        return {};
    }
    return dir;
}

// RAII cleanup for a temp directory tree; runs even when a fatal assertion fires.
class TempDirRemover {
  public:
    explicit TempDirRemover(fs::path path) : m_path(std::move(path)) {}
    ~TempDirRemover() {
        std::error_code error;
        fs::remove_all(m_path, error);
    }
    TempDirRemover(const TempDirRemover&) = delete;
    TempDirRemover& operator=(const TempDirRemover&) = delete;

  private:
    fs::path m_path;
};

// Counts loader shadow copies (".hotplugpp-shadow-*") directly inside dir.
int countShadowFiles(const fs::path& dir) {
    int count = 0;
    std::error_code error;
    for (fs::directory_iterator it(dir, error), end; !error && it != end; it.increment(error)) {
        const std::string name = it->path().filename().string();
        if (name.rfind(kShadowPrefix, 0) == 0) {
            ++count;
        }
    }
    return count;
}

} // namespace

class PluginLoaderTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_testPluginPath = std::string(TEST_PLUGIN_DIR) + "/" + SHARED_LIB_PREFIX + "test_plugin" + SHARED_LIB_SUFFIX;
        m_failingPluginPath = std::string(TEST_PLUGIN_DIR) + "/" + SHARED_LIB_PREFIX + "failing_plugin" + SHARED_LIB_SUFFIX;
    }

    void TearDown() override {
        // Ensure plugin is unloaded after each test
    }

    std::string m_testPluginPath;
    std::string m_failingPluginPath;
};

// ============================================================================
// Initial State Tests
// ============================================================================

TEST_F(PluginLoaderTest, InitialStateNotLoaded) {
    PluginLoader loader;
    EXPECT_FALSE(loader.isLoaded());
}

TEST_F(PluginLoaderTest, InitialStateNullPlugin) {
    PluginLoader loader;
    EXPECT_EQ(loader.getPlugin(), nullptr);
}

TEST_F(PluginLoaderTest, InitialStateEmptyPath) {
    PluginLoader loader;
    EXPECT_TRUE(loader.getPluginPath().empty());
}

// ============================================================================
// Load Plugin Tests
// ============================================================================

TEST_F(PluginLoaderTest, LoadValidPlugin) {
    PluginLoader loader;
    EXPECT_TRUE(loader.loadPlugin(m_testPluginPath));
    EXPECT_TRUE(loader.isLoaded());
}

TEST_F(PluginLoaderTest, LoadPluginReturnsValidInstance) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    EXPECT_STREQ(plugin->getName(), "TestPlugin");
}

TEST_F(PluginLoaderTest, LoadPluginSetsPath) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    EXPECT_EQ(loader.getPluginPath(), m_testPluginPath);
}

TEST_F(PluginLoaderTest, LoadNonExistentPlugin) {
    PluginLoader loader;
    EXPECT_FALSE(loader.loadPlugin(getDefinitelyMissingPluginPath()));
    EXPECT_FALSE(loader.isLoaded());
}

TEST_F(PluginLoaderTest, LoadInvalidFile) {
    // Create a temporary invalid file
    std::string invalidPath = getInvalidPluginPath();
    std::ofstream file(invalidPath);
    file << "This is not a valid shared library";
    file.close();
    
    PluginLoader loader;
    EXPECT_FALSE(loader.loadPlugin(invalidPath));
    EXPECT_FALSE(loader.isLoaded());
    
    std::remove(invalidPath.c_str());
}

TEST_F(PluginLoaderTest, LoadPluginWithFailingOnLoad) {
    PluginLoader loader;
    EXPECT_FALSE(loader.loadPlugin(m_failingPluginPath));
    EXPECT_FALSE(loader.isLoaded());
}

TEST_F(PluginLoaderTest, LoadPluginTwiceUnloadsPrevious) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    IPlugin* firstPlugin = loader.getPlugin();
    ASSERT_NE(firstPlugin, nullptr);
    
    // Load again - should unload previous and load new
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    // Should still be loaded
    EXPECT_TRUE(loader.isLoaded());
    EXPECT_NE(loader.getPlugin(), nullptr);
}

// ============================================================================
// Unload Plugin Tests
// ============================================================================

TEST_F(PluginLoaderTest, UnloadLoadedPlugin) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    ASSERT_TRUE(loader.isLoaded());
    
    loader.unloadPlugin();
    
    EXPECT_FALSE(loader.isLoaded());
    EXPECT_EQ(loader.getPlugin(), nullptr);
}

TEST_F(PluginLoaderTest, UnloadWhenNotLoaded) {
    PluginLoader loader;
    EXPECT_FALSE(loader.isLoaded());
    
    // Should not throw or crash
    loader.unloadPlugin();
    
    EXPECT_FALSE(loader.isLoaded());
}

TEST_F(PluginLoaderTest, UnloadTwice) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    loader.unloadPlugin();
    EXPECT_FALSE(loader.isLoaded());
    
    // Second unload should be safe
    loader.unloadPlugin();
    EXPECT_FALSE(loader.isLoaded());
}

// ============================================================================
// Plugin Interface Tests
// ============================================================================

TEST_F(PluginLoaderTest, PluginVersionIsCorrect) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    
    Version version = plugin->getVersion();
    EXPECT_EQ(version.major, 1);
    EXPECT_EQ(version.minor, 2);
    EXPECT_EQ(version.patch, 3);
}

TEST_F(PluginLoaderTest, PluginDescriptionIsCorrect) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    
    EXPECT_STREQ(plugin->getDescription(), "A test plugin for unit tests");
}

TEST_F(PluginLoaderTest, PluginOnUpdateWorks) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    
    // Should not throw or crash
    plugin->onUpdate(0.016f);
    plugin->onUpdate(0.016f);
    plugin->onUpdate(0.016f);
}

// ============================================================================
// Reload Callback Tests
// ============================================================================

TEST_F(PluginLoaderTest, SetReloadCallbackAcceptsNullptr) {
    PluginLoader loader;
    // Should not throw or crash
    loader.setReloadCallback(nullptr);
}

TEST_F(PluginLoaderTest, SetReloadCallbackAcceptsLambda) {
    PluginLoader loader;
    bool callbackCalled = false;
    
    loader.setReloadCallback([&callbackCalled]() {
        callbackCalled = true;
    });
    
    // Callback should not be called just by setting it
    EXPECT_FALSE(callbackCalled);
}

// ============================================================================
// Check And Reload Tests
// ============================================================================

TEST_F(PluginLoaderTest, CheckAndReloadWhenNotLoaded) {
    PluginLoader loader;

    // Should return NoChange and not crash when no plugin is loaded
    EXPECT_EQ(loader.checkAndReload(), PluginLoader::ReloadResult::NoChange);
}

TEST_F(PluginLoaderTest, CheckAndReloadNoChange) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));

    // Wait a bit and check - should not reload since file hasn't changed
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(loader.checkAndReload(), PluginLoader::ReloadResult::NoChange);
    EXPECT_TRUE(loader.isLoaded());
}

TEST_F(PluginLoaderTest, CheckAndReloadAfterFileChangeReloadsPluginOnce) {
    const fs::path pluginCopyPath = makePluginCopyForReloadTest(m_testPluginPath);
    ASSERT_FALSE(pluginCopyPath.empty());

    PluginLoader loader;
    int callbackCount = 0;
    loader.setReloadCallback([&callbackCount]() {
        ++callbackCount;
    });

    ASSERT_TRUE(loader.loadPlugin(pluginCopyPath.string()));
    ASSERT_TRUE(touchPluginFile(pluginCopyPath));

    // Poll for up to 2 seconds: detection (~100ms) + debounce (250ms) + margin.
    bool reloaded = false;
    for (int attempt = 0; attempt < 20; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (loader.checkAndReload() == PluginLoader::ReloadResult::Reloaded) {
            reloaded = true;
            break;
        }
    }

    EXPECT_TRUE(reloaded);
    EXPECT_EQ(callbackCount, 1);
    EXPECT_TRUE(loader.isLoaded());
    EXPECT_EQ(loader.checkAndReload(), PluginLoader::ReloadResult::NoChange);

    loader.unloadPlugin();
    std::error_code error;
    fs::remove(pluginCopyPath, error);
}

TEST_F(PluginLoaderTest, CheckAndReloadCoalescesRapidFileChanges) {
    const fs::path pluginCopyPath = makePluginCopyForReloadTest(m_testPluginPath);
    ASSERT_FALSE(pluginCopyPath.empty());

    PluginLoader loader;
    int callbackCount = 0;
    loader.setReloadCallback([&callbackCount]() {
        ++callbackCount;
    });

    ASSERT_TRUE(loader.loadPlugin(pluginCopyPath.string()));
    ASSERT_TRUE(touchPluginFile(pluginCopyPath));
    ASSERT_TRUE(touchPluginFile(pluginCopyPath));

    // Poll for up to 2 seconds: detection (~100ms) + debounce (250ms) + margin.
    bool reloaded = false;
    for (int attempt = 0; attempt < 20; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (loader.checkAndReload() == PluginLoader::ReloadResult::Reloaded) {
            reloaded = true;
            break;
        }
    }

    EXPECT_TRUE(reloaded);
    EXPECT_EQ(callbackCount, 1);
    EXPECT_TRUE(loader.isLoaded());
    EXPECT_EQ(loader.checkAndReload(), PluginLoader::ReloadResult::NoChange);

    loader.unloadPlugin();
    std::error_code error;
    fs::remove(pluginCopyPath, error);
}

TEST_F(PluginLoaderTest, CheckAndReloadWorksWhenWatcherRejectsThePath) {
    const fs::path pluginCopyPath = makePluginCopyForReloadTest(m_testPluginPath, ".bin");
    ASSERT_FALSE(pluginCopyPath.empty());

    PluginLoader loader;
    int callbackCount = 0;
    loader.setReloadCallback([&callbackCount]() {
        ++callbackCount;
    });

    ASSERT_TRUE(loader.loadPlugin(pluginCopyPath.string()));
    ASSERT_TRUE(loader.isLoaded());
    ASSERT_TRUE(touchPluginFile(pluginCopyPath));

    // Poll for up to 2 seconds: detection via mtime polling (~100ms) + margin.
    bool reloaded = false;
    for (int attempt = 0; attempt < 20; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (loader.checkAndReload() == PluginLoader::ReloadResult::Reloaded) {
            reloaded = true;
            break;
        }
    }

    EXPECT_TRUE(reloaded);
    EXPECT_EQ(callbackCount, 1);
    EXPECT_TRUE(loader.isLoaded());

    loader.unloadPlugin();
    std::error_code error;
    fs::remove(pluginCopyPath, error);
}

// ============================================================================
// Shadow Copy / Windows DLL-lock Regression Tests
// ============================================================================

// Primary regression for the Windows DLL-lock bug: the build-output file must stay
// writable while the plugin is loaded, so a rebuild can overwrite it and trigger a
// hot-reload. Before shadow copying, LoadLibrary held an exclusive lock on the
// original file and this overwrite failed on Windows (ERROR_SHARING_VIOLATION).
TEST_F(PluginLoaderTest, ReloadAfterOverwritingLoadedLibrary) {
    const fs::path dir = makeUniqueTempSubdir();
    ASSERT_FALSE(dir.empty());
    TempDirRemover dirGuard(dir);

    const fs::path loadedPath =
        dir / (std::string(SHARED_LIB_PREFIX) + "reload_overwrite" + SHARED_LIB_SUFFIX);
    std::error_code error;
    fs::copy_file(m_testPluginPath, loadedPath, fs::copy_options::overwrite_existing, error);
    ASSERT_FALSE(error) << error.message();

    PluginLoader loader;
    int callbackCount = 0;
    loader.setReloadCallback([&callbackCount]() { ++callbackCount; });

    ASSERT_TRUE(loader.loadPlugin(loadedPath.string()));
    ASSERT_TRUE(loader.isLoaded());

    // Crux of the regression: overwrite the original file while the plugin is
    // loaded. This must succeed because the loader holds only the shadow copy.
    error.clear();
    fs::copy_file(m_testPluginPath, loadedPath, fs::copy_options::overwrite_existing, error);
    ASSERT_FALSE(error) << "loaded plugin file is locked (shadow copy missing?): "
                        << error.message();

    // Ensure the modification time advances so the change is detected.
    ASSERT_TRUE(touchPluginFile(loadedPath));

    bool reloaded = false;
    for (int attempt = 0; attempt < 30; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (loader.checkAndReload() == PluginLoader::ReloadResult::Reloaded) {
            reloaded = true;
            break;
        }
    }

    EXPECT_TRUE(reloaded);
    EXPECT_GE(callbackCount, 1);
    EXPECT_TRUE(loader.isLoaded());

    loader.unloadPlugin();
}

// The loader must clean up its co-located shadow copy when the plugin is unloaded.
TEST_F(PluginLoaderTest, ShadowCopyRemovedAfterUnload) {
    const fs::path dir = makeUniqueTempSubdir();
    ASSERT_FALSE(dir.empty());
    TempDirRemover dirGuard(dir);

    const fs::path loadedPath =
        dir / (std::string(SHARED_LIB_PREFIX) + "shadow_cleanup" + SHARED_LIB_SUFFIX);
    std::error_code error;
    fs::copy_file(m_testPluginPath, loadedPath, fs::copy_options::overwrite_existing, error);
    ASSERT_FALSE(error) << error.message();

    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(loadedPath.string()));

    // Exactly one shadow copy should exist alongside the original while loaded.
    EXPECT_EQ(countShadowFiles(dir), 1) << "expected one shadow copy while loaded";

    loader.unloadPlugin();

    // The shadow copy must be gone after unload (only the original remains).
    EXPECT_EQ(countShadowFiles(dir), 0) << "shadow copy not removed after unload";
}

// A failed load (e.g. a corrupt library) must not leave a shadow copy behind: the
// loader creates the shadow, the native load fails, and cleanup must remove it.
TEST_F(PluginLoaderTest, FailedLoadLeavesNoShadowCopy) {
    const fs::path dir = makeUniqueTempSubdir();
    ASSERT_FALSE(dir.empty());
    TempDirRemover dirGuard(dir);

    const fs::path badPath =
        dir / (std::string(SHARED_LIB_PREFIX) + "not_a_library" + SHARED_LIB_SUFFIX);
    {
        std::ofstream bad(badPath, std::ios::binary);
        bad << "this is not a valid shared library";
    }

    PluginLoader loader;
    EXPECT_FALSE(loader.loadPlugin(badPath.string()));
    EXPECT_FALSE(loader.isLoaded());
    EXPECT_EQ(countShadowFiles(dir), 0) << "shadow copy left behind after a failed load";
}

// ============================================================================
// Destructor Tests
// ============================================================================

TEST_F(PluginLoaderTest, DestructorUnloadsPlugin) {
    {
        PluginLoader loader;
        ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
        ASSERT_TRUE(loader.isLoaded());
        // Destructor should be called here
    }
    // No crash means destructor worked correctly
    SUCCEED();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(PluginLoaderTest, LoadEmptyPath) {
    PluginLoader loader;
    EXPECT_FALSE(loader.loadPlugin(""));
    EXPECT_FALSE(loader.isLoaded());
}

TEST_F(PluginLoaderTest, GetPluginPathAfterUnload) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    loader.unloadPlugin();
    
    // Path should still be the same even after unload (based on implementation)
    // The path is only cleared when isLoaded is false, but the string itself remains
    EXPECT_FALSE(loader.isLoaded());
}

TEST_F(PluginLoaderTest, LoadAfterFailedLoad) {
    PluginLoader loader;
    
    // First, try to load a non-existent plugin
    EXPECT_FALSE(loader.loadPlugin(getDefinitelyMissingPluginPath()));
    EXPECT_FALSE(loader.isLoaded());
    
    // Then load a valid plugin
    EXPECT_TRUE(loader.loadPlugin(m_testPluginPath));
    EXPECT_TRUE(loader.isLoaded());
}

TEST_F(PluginLoaderTest, MultipleLoadUnloadCycles) {
    PluginLoader loader;
    
    for (int i = 0; i < 5; ++i) {
        ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
        EXPECT_TRUE(loader.isLoaded());
        
        loader.unloadPlugin();
        EXPECT_FALSE(loader.isLoaded());
    }
}

} // namespace tests
} // namespace hotplugpp
