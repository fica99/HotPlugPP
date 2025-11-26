#pragma once

#include "i_plugin.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
typedef HMODULE LibraryHandle;
#else
#include <dlfcn.h>
typedef void* LibraryHandle;
#endif

namespace hotplugpp {

// Forward declaration
class FileWatcher;

/**
 * @brief Plugin metadata and handle
 */
struct PluginInfo {
    std::string path;
    LibraryHandle handle = nullptr;
    IPlugin* instance = nullptr;
    CreatePluginFunc createFunc = nullptr;
    DestroyPluginFunc destroyFunc = nullptr;
    std::chrono::system_clock::time_point lastModified;
    bool isLoaded = false;
};

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

    /**
     * @brief Enable automatic file watching for hot-reload
     *
     * When enabled, the plugin file will be monitored for changes
     * using an asynchronous file watcher. When changes are detected,
     * a reload will be pending. The actual reload occurs on the next
     * call to checkAndReload(), and then the reload callback is invoked.
     *
     * @param enable true to enable, false to disable
     */
    void enableAutoReload(bool enable);

    /**
     * @brief Check if automatic reload is enabled
     * @return true if auto-reload is enabled, false otherwise
     */
    bool isAutoReloadEnabled() const;

  private:
    PluginInfo m_pluginInfo;
    std::function<void()> m_reloadCallback;
    std::unique_ptr<FileWatcher> m_fileWatcher;
    std::atomic<bool> m_autoReloadEnabled{false};
    std::atomic<int> m_pendingReload{0};
    mutable std::mutex m_reloadMutex;

    /**
     * @brief Get the last modification time of a file
     * @param path File path
     * @return Last modification time
     */
    std::chrono::system_clock::time_point getFileModificationTime(const std::string& path);

    /**
     * @brief Load a shared library
     * @param path Library path
     * @return Library handle or nullptr on failure
     */
    LibraryHandle loadLibrary(const std::string& path);

    /**
     * @brief Unload a shared library
     * @param handle Library handle
     */
    void unloadLibrary(LibraryHandle handle);

    /**
     * @brief Get a function pointer from a library
     * @param handle Library handle
     * @param name Function name
     * @return Function pointer or nullptr on failure
     */
    void* getFunction(LibraryHandle handle, const std::string& name);

    /**
     * @brief Get the last error message from dynamic library loading
     * @return Error message string
     */
    std::string getLastError();

    /**
     * @brief Setup file watching for the currently loaded plugin
     */
    void setupFileWatch();

    /**
     * @brief Remove file watching for the currently loaded plugin
     */
    void removeFileWatch();

    /**
     * @brief Handle file change notification from the file watcher
     * @param filePath Path of the changed file
     */
    void onFileChanged(const std::string& filePath);

    /**
     * @brief Process any pending reload (called from main thread)
     * @return true if reload was performed, false otherwise
     */
    bool processPendingReload();
};

} // namespace hotplugpp