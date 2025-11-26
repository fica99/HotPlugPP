# HotPlugPP API Reference

Complete API documentation for the HotPlugPP plugin system.

## Table of Contents

- [Core Interfaces](#core-interfaces)
  - [IPlugin](#iplugin)
  - [Version](#version)
- [Plugin Loader](#plugin-loader)
  - [PluginLoader](#pluginloader)
- [Macros](#macros)
- [Platform Support](#platform-support)

---

## Core Interfaces

### IPlugin

Base interface that all plugins must implement.

**Header:** `hotplugpp/i_plugin.hpp`

```cpp
namespace hotplugpp {
    class IPlugin {
    public:
        virtual ~IPlugin() = default;
        virtual bool onLoad() = 0;
        virtual void onUnload() = 0;
        virtual void onUpdate(float deltaTime) = 0;
        virtual const char* getName() const = 0;
        virtual Version getVersion() const = 0;
        virtual const char* getDescription() const = 0;
    };
}
```

#### Methods

##### `bool onLoad()`

Called when the plugin is loaded. Use this to initialize resources, allocate memory, or set up state.

**Returns:** `true` if initialization succeeded, `false` to abort loading.

**Example:**
```cpp
bool onLoad() override {
    m_texture = loadTexture("plugin_texture.png");
    return m_texture != nullptr;
}
```

---

##### `void onUnload()`

Called before the plugin is unloaded. Clean up resources here.

**Note:** Always called, even if `onLoad()` returned false.

**Example:**
```cpp
void onUnload() override {
    delete m_texture;
    m_texture = nullptr;
}
```

---

##### `void onUpdate(float deltaTime)`

Called regularly (typically each frame). This is where your plugin's main logic runs.

**Parameters:**
- `deltaTime`: Time elapsed since last update in seconds

**Example:**
```cpp
void onUpdate(float deltaTime) override {
    m_position += m_velocity * deltaTime;
    if (m_position > 100.0f) {
        m_velocity *= -1.0f;
    }
}
```

---

##### `const char* getName() const`

**Returns:** The plugin's name as a C-string.

---

##### `Version getVersion() const`

**Returns:** The plugin's version.

---

##### `const char* getDescription() const`

**Returns:** A brief description of the plugin.

---

### Version

Version structure for semantic versioning and compatibility checking.

**Header:** `hotplugpp/i_plugin.hpp`

```cpp
struct Version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    
    Version(uint32_t maj = 1, uint32_t min = 0, uint32_t pat = 0);
    bool isCompatible(const Version& other) const;
    std::string toString() const;
};
```

#### Methods

##### `bool isCompatible(const Version& other) const`

Checks if this version is compatible with another version.

**Compatibility Rules:**
- Major versions must match exactly
- This minor version must be >= other's minor version
- Patch version is ignored

**Returns:** `true` if compatible, `false` otherwise.

---

##### `std::string toString() const`

**Returns:** Version as a string in format "major.minor.patch".

---

## Plugin Loader

### PluginLoader

Manages dynamic loading, unloading, and hot-reloading of plugins.

**Header:** `hotplugpp/plugin_loader.hpp`

```cpp
class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();
    
    bool loadPlugin(const std::string& path);
    void unloadPlugin();
    bool checkAndReload();
    IPlugin* getPlugin() const;
    bool isLoaded() const;
    std::string getPluginPath() const;
    void setReloadCallback(std::function<void()> callback);
};
```

#### Methods

##### `bool loadPlugin(const std::string& path)`

Loads a plugin from a shared library file.

**Parameters:**
- `path`: Path to the plugin library (.so/.dll/.dylib)

**Returns:** `true` if loading succeeded, `false` otherwise.

---

##### `void unloadPlugin()`

Unloads the currently loaded plugin.

---

##### `bool checkAndReload()`

Checks if the plugin file has been modified and reloads if necessary.

**Returns:** `true` if plugin was reloaded, `false` otherwise.

---

##### `IPlugin* getPlugin() const`

**Returns:** Pointer to the loaded plugin instance, or `nullptr` if no plugin is loaded.

---

##### `bool isLoaded() const`

**Returns:** `true` if a plugin is currently loaded, `false` otherwise.

---

##### `std::string getPluginPath() const`

**Returns:** Path of the currently loaded plugin, or empty string if none loaded.

---

##### `void setReloadCallback(std::function<void()> callback)`

Sets a callback function to be called when a plugin is hot-reloaded.

**Parameters:**
- `callback`: Function to call on reload

---

## Macros

### HOTPLUGPP_CREATE_PLUGIN(PluginClass)

Convenience macro to create the required factory functions for a plugin.

**Parameters:**
- `PluginClass`: The name of your plugin class

**Example:**
```cpp
class MyPlugin : public hotplugpp::IPlugin {
    // ... implementation ...
};

HOTPLUGPP_CREATE_PLUGIN(MyPlugin)
```

---

### HOTPLUGPP_API

Platform-specific export macro for shared library symbols.

**Expands to:**
- Windows: `__declspec(dllexport)`
- Unix/Linux: `__attribute__((visibility("default")))`

---

## Platform Support

### Platform-Specific Behavior

| Platform | Extension | Load Function | Symbol Function |
|----------|-----------|---------------|-----------------|
| Windows  | .dll      | LoadLibraryA  | GetProcAddress  |
| Linux    | .so       | dlopen        | dlsym           |
| macOS    | .dylib    | dlopen        | dlsym           |

---

## Thread Safety

⚠️ **Warning:** The current implementation is **not thread-safe**.

- Do not call loader methods from multiple threads
- Serialize access with mutexes for multi-threaded applications

---

For more information:
- [[Home]] - Overview
- [[Build Instructions|BUILD]] - Build instructions
- [[Tutorial|TUTORIAL]] - Step-by-step guide
