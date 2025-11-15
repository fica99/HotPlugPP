#include "hotplugpp/PluginLoader.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <plugin_path>" << std::endl;
    std::cout << "Example: " << programName << " ./lib/libsample_plugin.so" << std::endl;
    std::cout << std::endl;
    std::cout << "The host application will:" << std::endl;
    std::cout << "  1. Load the specified plugin" << std::endl;
    std::cout << "  2. Call the plugin's update() method in a loop" << std::endl;
    std::cout << "  3. Monitor the plugin file for changes and hot-reload if modified" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== HotPlugPP Example Host Application ===" << std::endl;
    std::cout << std::endl;

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string pluginPath = argv[1];

    // Create plugin loader
    hotplugpp::PluginLoader loader;

    // Set up reload callback
    loader.setReloadCallback([]() {
        std::cout << std::endl;
        std::cout << "*** Plugin has been reloaded! ***" << std::endl;
        std::cout << std::endl;
    });

    // Load the plugin
    std::cout << "Loading plugin from: " << pluginPath << std::endl;
    if (!loader.loadPlugin(pluginPath)) {
        std::cerr << "Failed to load plugin!" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "Plugin loaded successfully!" << std::endl;
    
    hotplugpp::IPlugin* plugin = loader.getPlugin();
    if (plugin) {
        std::cout << "  Name: " << plugin->getName() << std::endl;
        std::cout << "  Version: " << plugin->getVersion().toString() << std::endl;
        std::cout << "  Description: " << plugin->getDescription() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Starting update loop (hot-reload monitoring enabled)..." << std::endl;
    std::cout << "You can modify and recompile the plugin to see hot-reload in action!" << std::endl;
    std::cout << std::endl;

    // Main loop
    const float targetFPS = 60.0f;
    const float deltaTime = 1.0f / targetFPS;
    const auto frameDuration = std::chrono::microseconds(static_cast<long long>(deltaTime * 1000000));

    int frameCount = 0;
    while (true) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // Check for plugin reload every 60 frames (once per second at 60 FPS)
        if (frameCount % 60 == 0) {
            loader.checkAndReload();
        }

        // Update the plugin
        plugin = loader.getPlugin();
        if (plugin) {
            plugin->onUpdate(deltaTime);
        } else {
            std::cerr << "Plugin is not loaded!" << std::endl;
            break;
        }

        frameCount++;

        // Sleep to maintain target frame rate
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
        auto sleepTime = frameDuration - elapsed;
        
        if (sleepTime.count() > 0) {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    std::cout << std::endl;
    std::cout << "Shutting down..." << std::endl;
    
    return 0;
}
