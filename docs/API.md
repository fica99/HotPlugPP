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

```cpp
namespace hotplug {
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

**Example:**
```cpp
const char* getName() const override {
    return "MyAwesomePlugin";
}
```

---

##### `Version getVersion() const`

**Returns:** The plugin's version.

**Example:**
```cpp
Version getVersion() const override {
    return Version(1, 2, 3); // v1.2.3
}
```

---

##### `const char* getDescription() const`

**Returns:** A brief description of the plugin.

**Example:**
```cpp
const char* getDescription() const override {
    return "Provides advanced particle effects";
}
```

---

### Version

Version structure for semantic versioning and compatibility checking.

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

#### Constructors

##### `Version(uint32_t major = 1, uint32_t minor = 0, uint32_t patch = 0)`

Creates a version object.

**Parameters:**
- `major`: Major version (incompatible API changes)
- `minor`: Minor version (backwards-compatible features)
- `patch`: Patch version (backwards-compatible bug fixes)

---

#### Methods

##### `bool isCompatible(const Version& other) const`

Checks if this version is compatible with another version.

**Compatibility Rules:**
- Major versions must match exactly
- This minor version must be >= other's minor version
- Patch version is ignored

**Returns:** `true` if compatible, `false` otherwise.

**Example:**
```cpp
Version plugin(1, 5, 0);
Version required(1, 3, 0);

if (plugin.isCompatible(required)) {
    // Plugin version 1.5.0 is compatible with requirement 1.3.0
}
```

---

##### `std::string toString() const`

**Returns:** Version as a string in format "major.minor.patch".

---

## Plugin Loader

### PluginLoader

Manages dynamic loading, unloading, and hot-reloading of plugins.

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

#### Constructors

##### `PluginLoader()`

Creates a new plugin loader. No plugins are loaded initially.

---

#### Methods

##### `bool loadPlugin(const std::string& path)`

Loads a plugin from a shared library file.

**Parameters:**
- `path`: Path to the plugin library (.so/.dll/.dylib)

**Returns:** `true` if loading succeeded, `false` otherwise.

**Behavior:**
1. Unloads any currently loaded plugin
2. Opens the shared library
3. Finds `createPlugin` and `destroyPlugin` functions
4. Creates plugin instance
5. Calls `onLoad()`

**Example:**
```cpp
PluginLoader loader;
if (!loader.loadPlugin("./plugins/my_plugin.so")) {
    std::cerr << "Failed to load plugin!" << std::endl;
}
```

---

##### `void unloadPlugin()`

Unloads the currently loaded plugin.

**Behavior:**
1. Calls `onUnload()` on the plugin
2. Destroys the plugin instance
3. Closes the shared library

**Example:**
```cpp
loader.unloadPlugin();
```

---

##### `bool checkAndReload()`

Checks if the plugin file has been modified and reloads if necessary.

**Returns:** `true` if plugin was reloaded, `false` otherwise.

**Behavior:**
1. Checks file modification time
2. If changed, unloads old plugin
3. Loads new plugin from same path
4. Calls reload callback if set

**Example:**
```cpp
// In your update loop
if (loader.checkAndReload()) {
    std::cout << "Plugin was hot-reloaded!" << std::endl;
}
```

---

##### `IPlugin* getPlugin() const`

**Returns:** Pointer to the loaded plugin instance, or `nullptr` if no plugin is loaded.

**Example:**
```cpp
if (auto* plugin = loader.getPlugin()) {
    plugin->onUpdate(deltaTime);
}
```

---

##### `bool isLoaded() const`

**Returns:** `true` if a plugin is currently loaded, `false` otherwise.

**Example:**
```cpp
if (loader.isLoaded()) {
    loader.getPlugin()->onUpdate(deltaTime);
}
```

---

##### `std::string getPluginPath() const`

**Returns:** Path of the currently loaded plugin, or empty string if none loaded.

---

##### `void setReloadCallback(std::function<void()> callback)`

Sets a callback function to be called when a plugin is hot-reloaded.

**Parameters:**
- `callback`: Function to call on reload

**Example:**
```cpp
loader.setReloadCallback([]() {
    std::cout << "Plugin reloaded, reinitializing state..." << std::endl;
    // Reinitialize any plugin-dependent state
});
```

---

## Macros

### HOTPLUG_CREATE_PLUGIN(PluginClass)

Convenience macro to create the required factory functions for a plugin.

**Parameters:**
- `PluginClass`: The name of your plugin class

**Generates:**
- `createPlugin()`: Factory function that creates an instance
- `destroyPlugin()`: Factory function that destroys an instance

**Example:**
```cpp
class MyPlugin : public hotplug::IPlugin {
    // ... implementation ...
};

HOTPLUG_CREATE_PLUGIN(MyPlugin)
```

**Equivalent to:**
```cpp
extern "C" HOTPLUG_API hotplug::IPlugin* createPlugin() {
    return new MyPlugin();
}

extern "C" HOTPLUG_API void destroyPlugin(hotplug::IPlugin* plugin) {
    delete plugin;
}
```

---

### HOTPLUG_API

Platform-specific export macro for shared library symbols.

**Expands to:**
- Windows: `__declspec(dllexport)`
- Unix/Linux: `__attribute__((visibility("default")))`

---

## Platform Support

### Platform-Specific Behavior

#### Windows (.dll)
- Uses `LoadLibraryA()` and `GetProcAddress()`
- Requires `__declspec(dllexport)` for exported symbols
- File extension: `.dll`

#### Linux (.so)
- Uses `dlopen()` and `dlsym()`
- Requires `-ldl` linker flag
- File extension: `.so`
- Requires position-independent code (`-fPIC`)

#### macOS (.dylib)
- Uses `dlopen()` and `dlsym()`
- File extension: `.dylib`
- Similar to Linux behavior

---

## Type Definitions

```cpp
// Function pointer types for plugin factories
typedef hotplug::IPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(hotplug::IPlugin*);

// Platform-specific library handle
#ifdef _WIN32
    typedef HMODULE LibraryHandle;
#else
    typedef void* LibraryHandle;
#endif
```

---

## Error Handling

### Common Errors

**Plugin fails to load:**
- File doesn't exist or wrong path
- Missing dependencies
- Incompatible architecture (32-bit vs 64-bit)
- Missing export functions

**Hot-reload fails:**
- File locked by another process
- Compilation errors in new version
- File permission issues

**Runtime errors:**
- Plugin crashes in `onUpdate()`
- Memory leaks in plugin code
- Exception thrown from plugin

### Best Practices

1. **Always check return values:**
```cpp
if (!loader.loadPlugin(path)) {
    // Handle error
}
```

2. **Use RAII in plugins:**
```cpp
class MyPlugin : public IPlugin {
    std::unique_ptr<Resource> m_resource;
    
    bool onLoad() override {
        m_resource = std::make_unique<Resource>();
        return m_resource != nullptr;
    }
};
```

3. **Handle hot-reload gracefully:**
```cpp
loader.setReloadCallback([&]() {
    // Save state before reload
    // Restore state after reload
});
```

---

## Examples

### Basic Plugin

```cpp
#include "hotplug/IPlugin.hpp"
#include <iostream>

class SimplePlugin : public hotplug::IPlugin {
public:
    bool onLoad() override {
        std::cout << "SimplePlugin loaded!" << std::endl;
        return true;
    }

    void onUnload() override {
        std::cout << "SimplePlugin unloading..." << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Do work here
    }

    const char* getName() const override { return "SimplePlugin"; }
    hotplug::Version getVersion() const override { return {1, 0, 0}; }
    const char* getDescription() const override { return "A simple plugin"; }
};

HOTPLUG_CREATE_PLUGIN(SimplePlugin)
```

### Host Application

```cpp
#include "hotplug/PluginLoader.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <plugin.so>" << std::endl;
        return 1;
    }

    hotplug::PluginLoader loader;
    
    if (!loader.loadPlugin(argv[1])) {
        return 1;
    }

    // Main loop
    const float deltaTime = 1.0f / 60.0f;
    while (true) {
        auto start = std::chrono::steady_clock::now();
        
        // Update plugin
        if (auto* plugin = loader.getPlugin()) {
            plugin->onUpdate(deltaTime);
        }
        
        // Check for hot-reload
        loader.checkAndReload();
        
        // Sleep to maintain frame rate
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::this_thread::sleep_for(std::chrono::milliseconds(16) - elapsed);
    }

    return 0;
}
```

---

## Thread Safety

⚠️ **Warning:** The current implementation is **not thread-safe**.

- Do not call `loadPlugin()`, `unloadPlugin()`, or `checkAndReload()` from multiple threads
- Do not call plugin methods from multiple threads unless the plugin explicitly supports it
- For multi-threaded applications, serialize access with mutexes

---

## Performance Tips

1. **Hot-reload checking frequency:**
   - Don't check every frame
   - Once per second is usually sufficient
   - Adjust based on development workflow

2. **Plugin initialization:**
   - Keep `onLoad()` fast
   - Defer heavy initialization to first `onUpdate()` if needed

3. **Update performance:**
   - `onUpdate()` is called frequently - keep it optimized
   - Use profiling to identify bottlenecks

4. **Memory:**
   - Plugins share the host's heap
   - Be careful with global variables in plugins
   - Clean up completely in `onUnload()`

---

For more information, see:
- [README.md](../README.md) - Overview and quick start
- [BUILD.md](BUILD.md) - Building instructions
- [examples/](../examples/) - Working examples