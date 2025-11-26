# HotPlugPP

A lightweight, cross-platform plugin system in modern C++ with support for runtime dynamic loading and hot-reloading of shared libraries.

## Features

- 🔌 **Dynamic Loading**: Load and unload plugins at runtime
- 🔥 **Hot-Reloading**: Automatically detect and reload modified plugins without restarting
- 🌐 **Cross-Platform**: Works on Windows (.dll), Linux (.so), and macOS (.dylib)
- 🎯 **Clean Interface**: Simple, intuitive plugin API
- 🛠️ **Modern C++**: Uses C++17 features for clean, maintainable code
- 🚀 **Lightweight**: Minimal dependencies and overhead
- 📝 **Configurable Logging**: Built-in logging with level control (Debug/Release aware)

## Quick Start

```bash
# Build
mkdir build && cd build
cmake .. && cmake --build .

# Run example
./bin/host_app ./bin/libsample_plugin.so
```

See [BUILD](https://github.com/fica99/HotPlugPP/wiki/BUILD) for detailed build instructions and platform-specific guidance.

## Creating a Plugin

```cpp
#include "hotplugpp/i_plugin.hpp"

class MyPlugin : public hotplugpp::IPlugin {
public:
    bool onLoad() override { return true; }
    void onUnload() override {}
    void onUpdate(float deltaTime) override {}
    const char* getName() const override { return "MyPlugin"; }
    hotplugpp::Version getVersion() const override { return {1, 0, 0}; }
    const char* getDescription() const override { return "My first plugin"; }
};

HOTPLUGPP_CREATE_PLUGIN(MyPlugin)
```

See [TUTORIAL](https://github.com/fica99/HotPlugPP/wiki/TUTORIAL) for a complete step-by-step guide.

## Using the Plugin Loader

```cpp
#include "hotplugpp/plugin_loader.hpp"

int main() {
    hotplugpp::PluginLoader loader;
    if (!loader.loadPlugin("./libmy_plugin.so")) {
        return 1;
    }
    
    auto* plugin = loader.getPlugin();
    if (plugin) {
        plugin->onUpdate(0.016f);
    }
    loader.checkAndReload();  // Detects and reloads modified plugins
    
    return 0;
}
```

See [API](https://github.com/fica99/HotPlugPP/wiki/API) for complete API documentation.

## Logging

HotPlugPP includes built-in logging via [spdlog](https://github.com/gabime/spdlog). Logging behavior differs based on build type:

- **Debug builds**: All log levels enabled (trace, debug, info, warn, error, critical)
- **Release builds**: Only warn, error, and critical levels are active by default

### Configuring Log Level

```cpp
#include "hotplugpp/logger.hpp"

// Set log level at runtime
hotplugpp::Logger::instance().setLevel(hotplugpp::LogLevel::Debug);

// Initialize with custom pattern
hotplugpp::Logger::instance().init(hotplugpp::LogLevel::Info, "[%l] %v");
```

### Using Logging Macros

```cpp
#include "hotplugpp/logger.hpp"

HOTPLUGPP_LOG_DEBUG("Plugin file modified, reloading...");
HOTPLUGPP_LOG_INFO("Plugin loaded: " + pluginName);
HOTPLUGPP_LOG_ERROR("Failed to load plugin: " + path);
```

To enable full logging in Release builds, define `HOTPLUGPP_ENABLE_LOGGING` before including the header.

## Platform Support

| Platform | Library Extension |    Status     |
|----------|-------------------|---------------|
| Linux    | .so               | ✅ Supported |
| Windows  | .dll              | ✅ Supported |
| macOS    | .dylib            | ✅ Supported |

## Requirements

- CMake 3.15+
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

## Documentation

📖 **[Wiki](https://github.com/fica99/HotPlugPP/wiki)** — Full documentation

- **[BUILD](https://github.com/fica99/HotPlugPP/wiki/BUILD)** - Detailed build instructions for all platforms
- **[TUTORIAL](https://github.com/fica99/HotPlugPP/wiki/TUTORIAL)** - Step-by-step plugin creation guide
- **[API](https://github.com/fica99/HotPlugPP/wiki/API)** - Complete API reference
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Contribution guidelines

## Examples

The `examples/` directory contains:
- `host_app.cpp` - Host application with hot-reload monitoring
- `sample_plugin/` - Simple plugin demonstrating basic features
- `math_plugin/` - Complex plugin with state management

Run example with hot-reload:
```bash
# Terminal 1: Run host
./bin/host_app ./bin/libsample_plugin.so

# Terminal 2: Modify and rebuild
# Edit examples/sample_plugin/SamplePlugin.cpp
cmake --build . --target sample_plugin
# Watch Terminal 1 for hot-reload!
```

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

**Code Formatting**: Run `./scripts/format-code.sh` (Linux/macOS) or `scripts\format-code.bat` (Windows) before submitting.

## License

See [LICENSE](LICENSE) file for details.
