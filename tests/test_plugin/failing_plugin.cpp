#include "hotplugpp/i_plugin.hpp"

/**
 * @brief A test plugin that fails to load (onLoad returns false)
 */
class FailingPlugin : public hotplugpp::IPlugin {
  public:
    FailingPlugin() = default;
    ~FailingPlugin() override = default;

    bool onLoad() override {
        // Intentionally fail initialization
        return false;
    }

    void onUnload() override {}

    void onUpdate(float deltaTime) override {
        (void)deltaTime;
    }

    const char* getName() const override { return "FailingPlugin"; }

    hotplugpp::Version getVersion() const override { return hotplugpp::Version(0, 0, 1); }

    const char* getDescription() const override {
        return "A plugin that intentionally fails to load";
    }
};

HOTPLUGPP_CREATE_PLUGIN(FailingPlugin)
