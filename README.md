# HotPlugPP

A lightweight, cross-platform plugin system in modern C++ with support for runtime dynamic loading and hot-reloading of shared libraries.

## Features

- 🔌 **Dynamic Loading**: Load and unload plugins at runtime
- 🔥 **Hot-Reloading**: Automatically detect and reload modified plugins without restarting
- 👁️ **Asynchronous File Watching**: Event-driven file monitoring using [efsw](https://github.com/SpartanJ/efsw)
- 🌐 **Cross-Platform**: Works on Windows (.dll), Linux (.so), and macOS (.dylib)
- 🎯 **Clean Interface**: Simple, intuitive plugin API
- 🛠️ **Modern C++**: Uses C++17 features for clean, maintainable code
- 🚀 **Lightweight**: Minimal dependencies and overhead

## Quick Start

```bash
# Build
mkdir build && cd build
cmake .. && cmake --build .

# Run example with polling-based hot-reload
./bin/host_app ./bin/libsample_plugin.so

# Run example with asynchronous file watching (more efficient)
./bin/host_app ./bin/libsample_plugin.so --auto-reload
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
    
    // Enable asynchronous file watching for efficient hot-reload
    loader.enableAutoReload(true);
    
    if (!loader.loadPlugin("./libmy_plugin.so")) {
        return 1;
    }
    
    auto* plugin = loader.getPlugin();
    if (plugin) {
        plugin->onUpdate(0.016f);
    }
    
    // With auto-reload enabled, this processes pending notifications
    // Without auto-reload, this polls the file modification time
    loader.checkAndReload();
    
    return 0;
}
```

See [API](https://github.com/fica99/HotPlugPP/wiki/API) for complete API documentation.

## Hot-Reload Modes

HotPlugPP supports two modes of file monitoring for hot-reload:

### 1. Polling-based (default)
- Uses file modification timestamps to detect changes
- `checkAndReload()` polls the file system periodically
- Simple and reliable, but requires periodic polling

### 2. Asynchronous File Watching (recommended)
- Uses [efsw](https://github.com/SpartanJ/efsw) for event-driven file monitoring
- Runs in a separate thread, notifying when files change
- More efficient as it doesn't require polling
- Enable with `loader.enableAutoReload(true)`

## Platform Support

| Platform | Library Extension |    Status     |
|----------|-------------------|---------------|
| Linux    | .so               | ✅ Supported |
| Windows  | .dll              | ✅ Supported |
| macOS    | .dylib            | ✅ Supported |

## Requirements

- CMake 3.15+
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

## Dependencies

- [efsw](https://github.com/SpartanJ/efsw) - Cross-platform file system watcher (automatically fetched via CMake)

## Documentation

📖 **[Wiki](https://github.com/fica99/HotPlugPP/wiki)** — Full documentation

- **[BUILD](https://github.com/fica99/HotPlugPP/wiki/BUILD)** - Detailed build instructions for all platforms
- **[TUTORIAL](https://github.com/fica99/HotPlugPP/wiki/TUTORIAL)** - Step-by-step plugin creation guide
- **[API](https://github.com/fica99/HotPlugPP/wiki/API)** - Complete API reference
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Contribution guidelines

## Examples

The `examples/` directory contains:
- `host_app.cpp` - Host application with hot-reload monitoring (supports both modes)
- `sample_plugin/` - Simple plugin demonstrating basic features
- `math_plugin/` - Complex plugin with state management

Run example with hot-reload:
```bash
# Terminal 1: Run host with asynchronous file watching
./bin/host_app ./bin/libsample_plugin.so --auto-reload

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
