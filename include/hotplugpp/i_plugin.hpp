#pragma once

#include <cstdint>
#include <string>

namespace hotplugpp {

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
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    // Comparison operators for easier version checking
    bool operator==(const Version& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    bool operator!=(const Version& other) const { return !(*this == other); }

    bool operator<(const Version& other) const {
        if (major != other.major)
            return major < other.major;
        if (minor != other.minor)
            return minor < other.minor;
        return patch < other.patch;
    }

    bool operator>(const Version& other) const { return other < *this; }

    bool operator<=(const Version& other) const { return !(other < *this); }

    bool operator>=(const Version& other) const { return !(*this < other); }
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

} // namespace hotplugpp

// Plugin creation/destruction function types
extern "C" {
typedef hotplugpp::IPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(hotplugpp::IPlugin*);
}

// Macro to simplify plugin implementation
#define HOTPLUGPP_PLUGIN_EXPORT extern "C"

#ifdef _WIN32
#define HOTPLUGPP_API __declspec(dllexport)
#else
#define HOTPLUGPP_API __attribute__((visibility("default")))
#endif

// Convenience macro for plugin factory functions
#define HOTPLUGPP_CREATE_PLUGIN(PluginClass) \
    HOTPLUGPP_PLUGIN_EXPORT HOTPLUGPP_API hotplugpp::IPlugin* createPlugin() { \
        return new PluginClass(); \
    } \
    HOTPLUGPP_PLUGIN_EXPORT HOTPLUGPP_API void destroyPlugin(hotplugpp::IPlugin* plugin) { \
        delete plugin; \
    }
