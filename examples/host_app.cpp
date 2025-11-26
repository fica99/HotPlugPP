#include "hotplugpp/plugin_loader.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <plugin_path> [--auto-reload]" << std::endl;
    std::cout << "Example: " << programName << " ./lib/libsample_plugin.so" << std::endl;
    std::cout << "         " << programName << " ./lib/libsample_plugin.so --auto-reload"
              << std::endl;
    std::cout << std::endl;
    std::cout << "The host application will:" << std::endl;
    std::cout << "  1. Load the specified plugin" << std::endl;
    std::cout << "  2. Call the plugin's update() method in a loop" << std::endl;
    std::cout << "  3. Monitor the plugin file for changes and hot-reload if modified" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --auto-reload  Use asynchronous file watching via efsw (more efficient)"
              << std::endl;
    std::cout << "                 Without this flag, uses polling-based reload detection"
              << std::endl;
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
    bool useAutoReload = false;

    // Check for --auto-reload flag
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--auto-reload") == 0) {
            useAutoReload = true;
            break;
        }
    }

    // Create plugin loader
    hotplugpp::PluginLoader loader;

    // Set up reload callback
    loader.setReloadCallback([]() {
        std::cout << std::endl;
        std::cout << "*** Plugin has been reloaded! ***" << std::endl;
        std::cout << std::endl;
    });

    // Enable auto-reload if requested
    if (useAutoReload) {
        std::cout << "Enabling asynchronous file watching (efsw)..." << std::endl;
        loader.enableAutoReload(true);
    }

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
    if (useAutoReload) {
        std::cout << "Starting update loop (asynchronous hot-reload enabled via efsw)..."
                  << std::endl;
    } else {
        std::cout << "Starting update loop (polling-based hot-reload monitoring)..." << std::endl;
    }
    std::cout << "You can modify and recompile the plugin to see hot-reload in action!"
              << std::endl;
    std::cout << std::endl;

    // Main loop
    const float targetFPS = 60.0f;
    const float deltaTime = 1.0f / targetFPS;
    const auto frameDuration = std::chrono::microseconds(
        static_cast<long long>(deltaTime * 1000000));

    uint64_t frameCount = 0;
    while (true) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // Check for plugin reload
        // With auto-reload enabled, this will process pending notifications
        // Without auto-reload, this polls the file modification time
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
