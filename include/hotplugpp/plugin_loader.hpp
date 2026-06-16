#pragma once

#include "i_plugin.hpp"

#include <functional>
#include <memory>
#include <string>

namespace hotplugpp {

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
     * @param path Path to the plugin
     * library (.so/.dll/.dylib)
     * @return true if loading succeeded, false otherwise. On
     * success, HotPlugPP also attempts
     * to start automatic file watching for that plugin
     * path.
     */
    bool loadPlugin(const std::string& path);

    /**
     * @brief Unload the currently loaded plugin
     */
    void unloadPlugin();

    /**
     * @brief Result of a checkAndReload() call.
     *
     * Distinguishes between "nothing happened", "success", and "failure"
     * so callers can react differently when a reload attempt fails (e.g. the
     * plugin binary was corrupt or missing at reload time).
     */
    enum class ReloadResult {
        NoChange,    ///< No reload was necessary (file unchanged or plugin not loaded).
        Reloaded,    ///< Plugin was successfully reloaded.
        ReloadFailed ///< File changed but reload attempt failed; plugin is now unloaded.
    };

    /// Apply a queued watcher event or direct file change and reload if necessary.
    ///
    /// Must be called from the host thread. The background watcher only marks a
    /// reload as pending; all actual unload/reload work happens here.
    ///
    /// @return ReloadResult::Reloaded   if the plugin was successfully reloaded.
    ///         ReloadResult::ReloadFailed if reload was attempted but failed (plugin unloaded).
    ///         ReloadResult::NoChange   if no reload was necessary.
    ReloadResult checkAndReload();

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
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace hotplugpp
