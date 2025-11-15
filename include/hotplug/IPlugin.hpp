#ifndef HOTPLUG_IPLUGIN_HPP
#define HOTPLUG_IPLUGIN_HPP

#include <string>
#include <cstdint>

namespace hotplug {

/**
 * @brief Version structure for plugin compatibility checking
 */
struct Version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;

    Version(uint32_t maj = 1, uint32_t min = 0, uint32_t pat = 0)
        : major(maj), minor(min), patch(pat) {}

    bool isCompatible(const Version& other) const {
        // Major version must match, minor version must be >= required
        return major == other.major && minor >= other.minor;
    }

    std::string toString() const {
        return std::to_string(major) + "." + 
               std::to_string(minor) + "." + 
               std::to_string(patch);
    }
};

/**
 * @brief Base interface for all plugins
 * 
 * This interface defines the contract that all plugins must implement.
 * It provides lifecycle management and metadata access.
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief Initialize the plugin
     * @return true if initialization succeeded, false otherwise
     */
    virtual bool onLoad() = 0;

    /**
     * @brief Cleanup the plugin before unloading
     */
    virtual void onUnload() = 0;

    /**
     * @brief Update the plugin (called each frame/tick)
     * @param deltaTime Time elapsed since last update in seconds
     */
    virtual void onUpdate(float deltaTime) = 0;

    /**
     * @brief Get the plugin name
     * @return Plugin name string
     */
    virtual const char* getName() const = 0;

    /**
     * @brief Get the plugin version
     * @return Plugin version
     */
    virtual Version getVersion() const = 0;

    /**
     * @brief Get the plugin description
     * @return Plugin description string
     */
    virtual const char* getDescription() const = 0;
};

} // namespace hotplug

// Plugin creation/destruction function types
extern "C" {
    typedef hotplug::IPlugin* (*CreatePluginFunc)();
    typedef void (*DestroyPluginFunc)(hotplug::IPlugin*);
}

// Macro to simplify plugin implementation
#define HOTPLUG_PLUGIN_EXPORT extern "C"

#ifdef _WIN32
    #define HOTPLUG_API __declspec(dllexport)
#else
    #define HOTPLUG_API __attribute__((visibility("default")))
#endif

// Convenience macro for plugin factory functions
#define HOTPLUG_CREATE_PLUGIN(PluginClass) \
    HOTPLUG_PLUGIN_EXPORT HOTPLUG_API hotplug::IPlugin* createPlugin() { \
        return new PluginClass(); \
    } \
    HOTPLUG_PLUGIN_EXPORT HOTPLUG_API void destroyPlugin(hotplug::IPlugin* plugin) { \
        delete plugin; \
    }

#endif // HOTPLUG_IPLUGIN_HPP
