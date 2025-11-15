# HotPlugPP

A lightweight, cross-platform plugin system in modern C++ with support for runtime dynamic loading and hot-reloading of shared libraries (.so/.dll).

## Features

- 🔌 **Dynamic Loading**: Load and unload plugins at runtime
- 🔥 **Hot-Reloading**: Automatically detect and reload modified plugins without restarting
- 🌐 **Cross-Platform**: Works on Windows (.dll), Linux (.so), and macOS (.dylib)
- 🎯 **Clean Interface**: Simple, intuitive plugin API
- 📦 **Header-Only Interface**: Easy integration into existing projects
- 🛠️ **Modern C++**: Uses C++17 features for clean, maintainable code
- 🚀 **Lightweight**: Minimal dependencies and overhead

## Use Cases

- Game engines and modding systems
- Desktop application plugins
- Development tools and extensible IDEs
- Server applications with dynamic modules
- Testing frameworks with runtime test discovery

## Quick Start

### Building the Project

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run the example
./bin/host_app ./bin/libsample_plugin.so
```

For detailed build instructions, see [docs/BUILD.md](docs/BUILD.md).

### Creating Your First Plugin

1. **Include the plugin interface:**

```cpp
#include "hotplugpp/IPlugin.hpp"
#include <iostream>

class MyPlugin : public hotplugpp::IPlugin {
public:
    bool onLoad() override {
        std::cout << "MyPlugin loaded!" << std::endl;
        return true;
    }

    void onUnload() override {
        std::cout << "MyPlugin unloading..." << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Called each frame/tick
    }

    const char* getName() const override {
        return "MyPlugin";
    }

    hotplugpp::Version getVersion() const override {
        return hotplugpp::Version(1, 0, 0);
    }

    const char* getDescription() const override {
        return "My first HotPlugPP plugin";
    }
};

// Export the plugin
HOTPLUGPP_CREATE_PLUGIN(MyPlugin)
```

2. **Build as a shared library:**

```cmake
add_library(my_plugin SHARED MyPlugin.cpp)
target_include_directories(my_plugin PRIVATE ${CMAKE_SOURCE_DIR}/include)
```

### Using the Plugin Loader

```cpp
#include "hotplugpp/PluginLoader.hpp"

int main() {
    hotplugpp::PluginLoader loader;
    
    // Load plugin
    if (!loader.loadPlugin("./libmy_plugin.so")) {
        return 1;
    }
    
    // Get plugin instance
    auto* plugin = loader.getPlugin();
    
    // Use the plugin
    plugin->onUpdate(0.016f); // 60 FPS
    
    // Check for hot-reload
    loader.checkAndReload();
    
    return 0;
}
```

## Architecture

### Plugin Interface (`IPlugin`)

All plugins must implement the `IPlugin` interface:

- `onLoad()` - Initialize plugin resources
- `onUnload()` - Clean up before unloading
- `onUpdate(deltaTime)` - Called each frame/tick
- `getName()` - Return plugin name
- `getVersion()` - Return version information
- `getDescription()` - Return plugin description

### Plugin Loader

The `PluginLoader` class manages the plugin lifecycle:

- **Loading**: Dynamically loads shared libraries and creates plugin instances
- **Unloading**: Safely unloads plugins and frees resources
- **Hot-Reloading**: Monitors plugin files and reloads on modification
- **Platform Abstraction**: Handles platform-specific library loading (LoadLibrary/dlopen)

## Hot-Reload Workflow

1. Modify plugin source code
2. Recompile the plugin (shared library)
3. `PluginLoader::checkAndReload()` detects file modification
4. Old plugin instance is unloaded
5. New plugin is loaded and initialized
6. Execution continues with updated plugin

## Platform Support

| Platform | Library Extension | Compiler       |
|----------|------------------|----------------|
| Linux    | .so              | GCC/Clang      |
| Windows  | .dll             | MSVC/MinGW     |
| macOS    | .dylib           | Clang          |

## Requirements

- CMake 3.15 or higher
- C++17 compatible compiler
  - GCC 7+
  - Clang 5+
  - MSVC 2017+

## Project Structure

```
HotPlugPP/
├── include/
│   └── hotplugpp/
│       ├── IPlugin.hpp          # Plugin interface
│       └── PluginLoader.hpp     # Plugin loader
├── src/
│   ├── PluginLoader.cpp         # Loader implementation
│   └── CMakeLists.txt
├── examples/
│   ├── host_app.cpp             # Example host application
│   ├── sample_plugin/
│   │   └── SamplePlugin.cpp     # Example plugin
│   └── CMakeLists.txt
├── CMakeLists.txt               # Root CMake file
└── README.md
```

## Advanced Usage

### Setting a Reload Callback

```cpp
loader.setReloadCallback([]() {
    std::cout << "Plugin was reloaded!" << std::endl;
});
```

### Version Compatibility Checking

```cpp
hotplugpp::Version required(1, 0, 0);
auto pluginVersion = plugin->getVersion();

if (pluginVersion.isCompatible(required)) {
    // Versions are compatible
}
```

## License

See [LICENSE](LICENSE) file for details.

## Examples

See the `examples/` directory for:
- `host_app.cpp` - Complete host application with hot-reload monitoring
- `sample_plugin/SamplePlugin.cpp` - Simple plugin implementation
- `math_plugin/MathPlugin.cpp` - Complex plugin with state management

Run the example to see hot-reloading in action:

```bash
# Terminal 1: Run the host app
./bin/host_app ./bin/libsample_plugin.so

# Terminal 2: Modify and rebuild the plugin
# Edit examples/sample_plugin/SamplePlugin.cpp
cmake --build . --target sample_plugin

# Watch Terminal 1 to see the plugin hot-reload!
```

## Documentation

- **[README.md](README.md)** - Project overview and quick start
- **[docs/BUILD.md](docs/BUILD.md)** - Detailed build instructions for all platforms
- **[docs/API.md](docs/API.md)** - Complete API reference
- **[docs/TUTORIAL.md](docs/TUTORIAL.md)** - Step-by-step plugin creation tutorial
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Contribution guidelines

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.

Areas where contributions are especially welcome:
- Additional example plugins
- Platform-specific testing (macOS, Windows)
- Multi-plugin support
- Performance improvements
- Documentation and tutorials
