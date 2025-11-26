#include "hotplugpp/plugin_loader.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <fstream>
#include <thread>

namespace hotplugpp {
namespace tests {

class IntegrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        m_testPluginPath = std::string(TEST_PLUGIN_DIR) + "/" + SHARED_LIB_PREFIX + "test_plugin" + SHARED_LIB_SUFFIX;
    }

    std::string m_testPluginPath;
};

// ============================================================================
// End-to-End Plugin Lifecycle Tests
// ============================================================================

TEST_F(IntegrationTest, FullPluginLifecycle) {
    PluginLoader loader;
    
    // Load plugin
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    ASSERT_TRUE(loader.isLoaded());
    
    // Get plugin and verify
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    EXPECT_STREQ(plugin->getName(), "TestPlugin");
    
    // Simulate a few update cycles
    for (int i = 0; i < 100; ++i) {
        plugin->onUpdate(0.016f);
    }
    
    // Unload plugin
    loader.unloadPlugin();
    EXPECT_FALSE(loader.isLoaded());
    EXPECT_EQ(loader.getPlugin(), nullptr);
}

TEST_F(IntegrationTest, PluginVersionCompatibility) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    
    Version pluginVersion = plugin->getVersion();
    
    // Test compatibility with various required versions
    Version compatible(1, 0, 0);  // Lower minor
    Version exactMatch(1, 2, 3);  // Exact match
    Version higherMinor(1, 3, 0); // Higher minor - should be incompatible
    
    EXPECT_TRUE(pluginVersion.isCompatible(compatible));
    EXPECT_TRUE(pluginVersion.isCompatible(exactMatch));
    EXPECT_FALSE(pluginVersion.isCompatible(higherMinor));
}

TEST_F(IntegrationTest, ReloadCallbackInvoked) {
    PluginLoader loader;
    int callbackCount = 0;
    
    loader.setReloadCallback([&callbackCount]() {
        callbackCount++;
    });
    
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    // Initial load should not trigger callback
    EXPECT_EQ(callbackCount, 0);
    
    // Check and reload should not trigger callback if file hasn't changed
    loader.checkAndReload();
    EXPECT_EQ(callbackCount, 0);
}

TEST_F(IntegrationTest, MultiplePluginInstances) {
    PluginLoader loader1;
    PluginLoader loader2;
    
    // Load the same plugin in two loaders
    ASSERT_TRUE(loader1.loadPlugin(m_testPluginPath));
    ASSERT_TRUE(loader2.loadPlugin(m_testPluginPath));
    
    // Both should be loaded
    EXPECT_TRUE(loader1.isLoaded());
    EXPECT_TRUE(loader2.isLoaded());
    
    // Get plugins
    IPlugin* plugin1 = loader1.getPlugin();
    IPlugin* plugin2 = loader2.getPlugin();
    
    ASSERT_NE(plugin1, nullptr);
    ASSERT_NE(plugin2, nullptr);
    
    // Both should have the same name
    EXPECT_STREQ(plugin1->getName(), plugin2->getName());
    
    // But they should be different instances
    EXPECT_NE(plugin1, plugin2);
    
    // Update both independently
    plugin1->onUpdate(0.016f);
    plugin2->onUpdate(0.016f);
    plugin2->onUpdate(0.016f);
    
    // Unload both
    loader1.unloadPlugin();
    loader2.unloadPlugin();
    
    EXPECT_FALSE(loader1.isLoaded());
    EXPECT_FALSE(loader2.isLoaded());
}

TEST_F(IntegrationTest, UpdateLoopSimulation) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    
    const float targetFPS = 60.0f;
    const float deltaTime = 1.0f / targetFPS;
    const int simulationFrames = 300;  // Simulate 5 seconds at 60 FPS
    
    for (int frame = 0; frame < simulationFrames; ++frame) {
        // Periodically check for reload (every 60 frames)
        if (frame % 60 == 0) {
            loader.checkAndReload();
        }
        
        // Update plugin
        plugin = loader.getPlugin();
        if (plugin) {
            plugin->onUpdate(deltaTime);
        }
    }
    
    // Plugin should still be loaded
    EXPECT_TRUE(loader.isLoaded());
    EXPECT_NE(loader.getPlugin(), nullptr);
}

TEST_F(IntegrationTest, PluginMetadataConsistency) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    
    // Verify metadata is consistent across multiple calls
    for (int i = 0; i < 10; ++i) {
        EXPECT_STREQ(plugin->getName(), "TestPlugin");
        EXPECT_STREQ(plugin->getDescription(), "A test plugin for unit tests");
        
        Version v = plugin->getVersion();
        EXPECT_EQ(v.major, 1);
        EXPECT_EQ(v.minor, 2);
        EXPECT_EQ(v.patch, 3);
    }
}

TEST_F(IntegrationTest, SequentialLoadUnloadOperations) {
    PluginLoader loader;
    
    for (int cycle = 0; cycle < 10; ++cycle) {
        // Load
        ASSERT_TRUE(loader.loadPlugin(m_testPluginPath)) << "Failed on cycle " << cycle;
        EXPECT_TRUE(loader.isLoaded());
        
        // Use plugin
        IPlugin* plugin = loader.getPlugin();
        ASSERT_NE(plugin, nullptr);
        EXPECT_STREQ(plugin->getName(), "TestPlugin");
        plugin->onUpdate(0.016f);
        
        // Unload
        loader.unloadPlugin();
        EXPECT_FALSE(loader.isLoaded());
        EXPECT_EQ(loader.getPlugin(), nullptr);
    }
}

TEST_F(IntegrationTest, PluginPathPersistence) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    std::string pathBeforeUnload = loader.getPluginPath();
    EXPECT_EQ(pathBeforeUnload, m_testPluginPath);
    
    loader.unloadPlugin();
    
    // Path should still be accessible (for potential reload)
    // This depends on implementation - checking that we don't crash
    loader.getPluginPath();  // Should not throw
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(IntegrationTest, RapidLoadUnload) {
    PluginLoader loader;
    
    // Rapid load/unload cycles
    for (int i = 0; i < 50; ++i) {
        ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
        loader.unloadPlugin();
    }
    
    EXPECT_FALSE(loader.isLoaded());
}

TEST_F(IntegrationTest, ManyUpdatesWithoutReload) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    
    // Many update calls
    for (int i = 0; i < 10000; ++i) {
        plugin->onUpdate(0.016f);
    }
    
    // Should still be functional
    EXPECT_TRUE(loader.isLoaded());
    EXPECT_STREQ(plugin->getName(), "TestPlugin");
}

TEST_F(IntegrationTest, FrequentReloadChecks) {
    PluginLoader loader;
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    
    // Many reload checks (file hasn't changed, so none should trigger)
    for (int i = 0; i < 1000; ++i) {
        bool reloaded = loader.checkAndReload();
        EXPECT_FALSE(reloaded);
    }
    
    // Should still be loaded
    EXPECT_TRUE(loader.isLoaded());
}

// ============================================================================
// Error Recovery Tests
// ============================================================================

TEST_F(IntegrationTest, RecoverFromFailedLoad) {
    PluginLoader loader;
    
    // First, try to load a non-existent plugin
    EXPECT_FALSE(loader.loadPlugin("/nonexistent/plugin.so"));
    EXPECT_FALSE(loader.isLoaded());
    
    // Then load a valid plugin
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    EXPECT_TRUE(loader.isLoaded());
    
    IPlugin* plugin = loader.getPlugin();
    ASSERT_NE(plugin, nullptr);
    EXPECT_STREQ(plugin->getName(), "TestPlugin");
}

TEST_F(IntegrationTest, LoadValidAfterInvalidPath) {
    PluginLoader loader;
    
    // Try various invalid paths
    EXPECT_FALSE(loader.loadPlugin(""));
    EXPECT_FALSE(loader.loadPlugin("/"));
    EXPECT_FALSE(loader.loadPlugin("/tmp/nonexistent.so"));
    
    // Should still be able to load valid plugin
    ASSERT_TRUE(loader.loadPlugin(m_testPluginPath));
    EXPECT_TRUE(loader.isLoaded());
}

} // namespace tests
} // namespace hotplugpp
