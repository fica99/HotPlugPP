#pragma once

#include "i_plugin.hpp"

#include <functional>
#include <memory>
#include <string>

namespace hotplugpp {

// Forward declaration for PIMPL pattern
struct PluginInfo;

/**
 * @brief Manages dynamic loading, unloading, and hot-reloading of plugins
 */
class PluginLoader {
  public:
    PluginLoader();
    ~PluginLoader();

    // Disable copy
    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;

    /**
     * @brief Load a plugin from a shared library
     * @param path Path to the plugin library (.so/.dll/.dylib)
     * @return true if loading succeeded, false otherwise
     */
    bool loadPlugin(const std::string& path);

    /**
     * @brief Unload the currently loaded plugin
     */
    void unloadPlugin();

    /**
     * @brief Check if plugin file has been modified and reload if necessary
     * @return true if plugin was reloaded, false otherwise
     */
    bool checkAndReload();

    /**
     * @brief Get the loaded plugin instance
     * @return Pointer to plugin instance or nullptr if not loaded
     */
    IPlugin* getPlugin() const;

    /**
     * @brief Check if a plugin is currently loaded
     * @return true if plugin is loaded, false otherwise
     */
    bool isLoaded() const;

    /**
     * @brief Get the path of the currently loaded plugin
     * @return Plugin path or empty string if not loaded
     */
    std::string getPluginPath() const;

    /**
     * @brief Set callback for when plugin is reloaded
     * @param callback Function to call when plugin is reloaded
     */
    void setReloadCallback(std::function<void()> callback);

  private:
    std::unique_ptr<PluginInfo> m_pluginInfo;
    std::function<void()> m_reloadCallback;
};

} // namespace hotplugpp