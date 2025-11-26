#include "hotplugpp/IPlugin.hpp"

#include <iostream>

/**
 * @brief A test plugin for unit tests
 */
class TestPlugin : public hotplugpp::IPlugin {
  public:
    TestPlugin() : m_loadCalled(false), m_unloadCalled(false), m_updateCount(0) {}

    ~TestPlugin() override = default;

    bool onLoad() override {
        m_loadCalled = true;
        return true;
    }

    void onUnload() override {
        m_unloadCalled = true;
    }

    void onUpdate(float deltaTime) override {
        m_updateCount++;
        m_lastDeltaTime = deltaTime;
    }

    const char* getName() const override { return "TestPlugin"; }

    hotplugpp::Version getVersion() const override { return hotplugpp::Version(1, 2, 3); }

    const char* getDescription() const override {
        return "A test plugin for unit tests";
    }

  private:
    bool m_loadCalled;
    bool m_unloadCalled;
    int m_updateCount;
    float m_lastDeltaTime;
};

HOTPLUGPP_CREATE_PLUGIN(TestPlugin)
