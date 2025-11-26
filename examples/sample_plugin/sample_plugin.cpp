#include "hotplugpp/i_plugin.hpp"

#include <cstdint>
#include <iostream>

/**
 * @brief A simple example plugin that demonstrates the plugin interface
 */
class SamplePlugin : public hotplugpp::IPlugin {
  public:
    SamplePlugin() : m_counter(0), m_totalTime(0.0f) {
        std::cout << "[SamplePlugin] Constructor called" << std::endl;
    }

    ~SamplePlugin() override { std::cout << "[SamplePlugin] Destructor called" << std::endl; }

    bool onLoad() override {
        std::cout << "[SamplePlugin] onLoad() - Initializing plugin..." << std::endl;
        std::cout << "[SamplePlugin] Plugin is ready!" << std::endl;
        return true;
    }

    void onUnload() override {
        std::cout << "[SamplePlugin] onUnload() - Cleaning up..." << std::endl;
        std::cout << "[SamplePlugin] Total updates: " << m_counter << std::endl;
        std::cout << "[SamplePlugin] Total time: " << m_totalTime << " seconds" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        m_counter++;
        m_totalTime += deltaTime;

        // Print a message every 60 updates (approximately 1 second at 60 FPS)
        if (m_counter % 60 == 0) {
            std::cout << "[SamplePlugin] Update #" << m_counter << " - Running for " << m_totalTime
                      << " seconds" << std::endl;
        }
    }

    const char* getName() const override { return "SamplePlugin"; }

    hotplugpp::Version getVersion() const override { return hotplugpp::Version(1, 0, 0); }

    const char* getDescription() const override {
        return "A simple example plugin demonstrating the HotPlugPP interface";
    }

  private:
    uint64_t m_counter;
    float m_totalTime;
};

// Use the convenience macro to create factory functions
HOTPLUGPP_CREATE_PLUGIN(SamplePlugin)
