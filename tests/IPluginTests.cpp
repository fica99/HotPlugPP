#include "hotplugpp/IPlugin.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace hotplugpp {
namespace tests {

// Mock implementation of IPlugin for testing the interface
class MockPlugin : public IPlugin {
  public:
    MockPlugin() : m_loadCalled(false), m_unloadCalled(false), m_updateCount(0), m_lastDeltaTime(0.0f) {}

    bool onLoad() override {
        m_loadCalled = true;
        return m_onLoadReturnValue;
    }

    void onUnload() override {
        m_unloadCalled = true;
    }

    void onUpdate(float deltaTime) override {
        m_updateCount++;
        m_lastDeltaTime = deltaTime;
    }

    const char* getName() const override { return "MockPlugin"; }

    Version getVersion() const override { return m_version; }

    const char* getDescription() const override {
        return "A mock plugin for testing";
    }

    // Test helpers
    bool wasLoadCalled() const { return m_loadCalled; }
    bool wasUnloadCalled() const { return m_unloadCalled; }
    int getUpdateCount() const { return m_updateCount; }
    float getLastDeltaTime() const { return m_lastDeltaTime; }
    
    void setOnLoadReturnValue(bool value) { m_onLoadReturnValue = value; }
    void setVersion(const Version& version) { m_version = version; }
    void reset() {
        m_loadCalled = false;
        m_unloadCalled = false;
        m_updateCount = 0;
        m_lastDeltaTime = 0.0f;
    }

  private:
    bool m_loadCalled;
    bool m_unloadCalled;
    int m_updateCount;
    float m_lastDeltaTime;
    bool m_onLoadReturnValue = true;
    Version m_version{1, 0, 0};
};

// ============================================================================
// IPlugin Interface Tests
// ============================================================================

TEST(IPluginTest, CanInstantiateMockPlugin) {
    MockPlugin plugin;
    EXPECT_STREQ(plugin.getName(), "MockPlugin");
}

TEST(IPluginTest, OnLoadCalledCorrectly) {
    MockPlugin plugin;
    EXPECT_FALSE(plugin.wasLoadCalled());
    
    plugin.onLoad();
    
    EXPECT_TRUE(plugin.wasLoadCalled());
}

TEST(IPluginTest, OnLoadReturnsTrue) {
    MockPlugin plugin;
    plugin.setOnLoadReturnValue(true);
    
    EXPECT_TRUE(plugin.onLoad());
}

TEST(IPluginTest, OnLoadReturnsFalse) {
    MockPlugin plugin;
    plugin.setOnLoadReturnValue(false);
    
    EXPECT_FALSE(plugin.onLoad());
}

TEST(IPluginTest, OnUnloadCalledCorrectly) {
    MockPlugin plugin;
    EXPECT_FALSE(plugin.wasUnloadCalled());
    
    plugin.onUnload();
    
    EXPECT_TRUE(plugin.wasUnloadCalled());
}

TEST(IPluginTest, OnUpdateIncrementsCounter) {
    MockPlugin plugin;
    EXPECT_EQ(plugin.getUpdateCount(), 0);
    
    plugin.onUpdate(0.016f);
    EXPECT_EQ(plugin.getUpdateCount(), 1);
    
    plugin.onUpdate(0.016f);
    EXPECT_EQ(plugin.getUpdateCount(), 2);
    
    plugin.onUpdate(0.016f);
    EXPECT_EQ(plugin.getUpdateCount(), 3);
}

TEST(IPluginTest, OnUpdateRecordsLastDeltaTime) {
    MockPlugin plugin;
    
    plugin.onUpdate(0.016f);
    EXPECT_FLOAT_EQ(plugin.getLastDeltaTime(), 0.016f);
    
    plugin.onUpdate(0.032f);
    EXPECT_FLOAT_EQ(plugin.getLastDeltaTime(), 0.032f);
    
    plugin.onUpdate(0.001f);
    EXPECT_FLOAT_EQ(plugin.getLastDeltaTime(), 0.001f);
}

TEST(IPluginTest, OnUpdateWithZeroDeltaTime) {
    MockPlugin plugin;
    
    plugin.onUpdate(0.0f);
    
    EXPECT_EQ(plugin.getUpdateCount(), 1);
    EXPECT_FLOAT_EQ(plugin.getLastDeltaTime(), 0.0f);
}

TEST(IPluginTest, OnUpdateWithNegativeDeltaTime) {
    MockPlugin plugin;
    
    // Negative delta time shouldn't crash, even if it's not meaningful
    plugin.onUpdate(-0.016f);
    
    EXPECT_EQ(plugin.getUpdateCount(), 1);
    EXPECT_FLOAT_EQ(plugin.getLastDeltaTime(), -0.016f);
}

TEST(IPluginTest, OnUpdateWithLargeDeltaTime) {
    MockPlugin plugin;
    
    plugin.onUpdate(100.0f);
    
    EXPECT_EQ(plugin.getUpdateCount(), 1);
    EXPECT_FLOAT_EQ(plugin.getLastDeltaTime(), 100.0f);
}

TEST(IPluginTest, GetNameReturnsCorrectValue) {
    MockPlugin plugin;
    EXPECT_STREQ(plugin.getName(), "MockPlugin");
}

TEST(IPluginTest, GetVersionReturnsCorrectValue) {
    MockPlugin plugin;
    plugin.setVersion(Version(2, 3, 4));
    
    Version v = plugin.getVersion();
    EXPECT_EQ(v.major, 2);
    EXPECT_EQ(v.minor, 3);
    EXPECT_EQ(v.patch, 4);
}

TEST(IPluginTest, GetDescriptionReturnsCorrectValue) {
    MockPlugin plugin;
    EXPECT_STREQ(plugin.getDescription(), "A mock plugin for testing");
}

TEST(IPluginTest, ResetClearsState) {
    MockPlugin plugin;
    
    plugin.onLoad();
    plugin.onUnload();
    plugin.onUpdate(0.016f);
    plugin.onUpdate(0.016f);
    
    plugin.reset();
    
    EXPECT_FALSE(plugin.wasLoadCalled());
    EXPECT_FALSE(plugin.wasUnloadCalled());
    EXPECT_EQ(plugin.getUpdateCount(), 0);
    EXPECT_FLOAT_EQ(plugin.getLastDeltaTime(), 0.0f);
}

// ============================================================================
// Polymorphism Tests
// ============================================================================

TEST(IPluginTest, PolymorphicAccess) {
    std::unique_ptr<IPlugin> plugin = std::make_unique<MockPlugin>();
    
    EXPECT_STREQ(plugin->getName(), "MockPlugin");
    EXPECT_TRUE(plugin->onLoad());
    plugin->onUpdate(0.016f);
    plugin->onUnload();
}

TEST(IPluginTest, VirtualDestructorWorks) {
    IPlugin* plugin = new MockPlugin();
    
    // This should not leak or crash
    delete plugin;
    
    SUCCEED();
}

// ============================================================================
// Full Lifecycle Tests
// ============================================================================

TEST(IPluginTest, FullLifecycle) {
    MockPlugin plugin;
    
    // Initial state
    EXPECT_FALSE(plugin.wasLoadCalled());
    EXPECT_FALSE(plugin.wasUnloadCalled());
    EXPECT_EQ(plugin.getUpdateCount(), 0);
    
    // Load
    EXPECT_TRUE(plugin.onLoad());
    EXPECT_TRUE(plugin.wasLoadCalled());
    
    // Multiple updates
    for (int i = 0; i < 100; ++i) {
        plugin.onUpdate(0.016f);
    }
    EXPECT_EQ(plugin.getUpdateCount(), 100);
    
    // Unload
    plugin.onUnload();
    EXPECT_TRUE(plugin.wasUnloadCalled());
}

TEST(IPluginTest, LoadUnloadMultipleTimes) {
    MockPlugin plugin;
    
    for (int i = 0; i < 5; ++i) {
        plugin.reset();
        
        EXPECT_TRUE(plugin.onLoad());
        EXPECT_TRUE(plugin.wasLoadCalled());
        
        plugin.onUpdate(0.016f);
        EXPECT_EQ(plugin.getUpdateCount(), 1);
        
        plugin.onUnload();
        EXPECT_TRUE(plugin.wasUnloadCalled());
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(IPluginTest, UpdateWithoutLoad) {
    MockPlugin plugin;
    
    // Update should still work even without onLoad being called
    plugin.onUpdate(0.016f);
    
    EXPECT_EQ(plugin.getUpdateCount(), 1);
    EXPECT_FALSE(plugin.wasLoadCalled());
}

TEST(IPluginTest, UnloadWithoutLoad) {
    MockPlugin plugin;
    
    // Unload should still work even without onLoad being called
    plugin.onUnload();
    
    EXPECT_TRUE(plugin.wasUnloadCalled());
    EXPECT_FALSE(plugin.wasLoadCalled());
}

TEST(IPluginTest, MultipleUnloads) {
    MockPlugin plugin;
    
    plugin.onLoad();
    plugin.onUnload();
    plugin.onUnload();  // Second unload
    plugin.onUnload();  // Third unload
    
    EXPECT_TRUE(plugin.wasUnloadCalled());
}

} // namespace tests
} // namespace hotplugpp
