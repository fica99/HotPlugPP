# HotPlugPP

A lightweight, cross-platform plugin system in modern C++ with support for runtime dynamic loading and hot-reloading of shared libraries.

## Features

- 🔌 **Dynamic Loading**: Load and unload plugins at runtime
- 🔥 **Watcher-Based Hot-Reloading**: Queue reloads from a background watcher and apply them on your host loop
- 🌐 **Cross-Platform**: Works on Windows (.dll), Linux (.so), and macOS (.dylib)
- 🎯 **Clean Interface**: Simple, intuitive plugin API
- 🛠️ **Modern C++**: Uses C++17 features for clean, maintainable code
- 🚀 **Pragmatic Fallbacks**: Uses `efsw` when available and falls back to built-in polling when it is not

## Quick Start

```bash
# Configure
cmake -S . -B build

# Build the host example and sample plugin explicitly
cmake --build build --config Release --target build_host_app build_sample_plugin

# Run example
./build/bin/host_app ./build/bin/libsample_plugin.so
```

See [BUILD](https://github.com/fica99/HotPlugPP/wiki/BUILD) for detailed build instructions and platform-specific guidance.

Examples and tests are configured with `EXCLUDE_FROM_ALL`, so they are not built by the default `cmake --build build` invocation. Build the targets you need explicitly.

## Watcher Configuration

HotPlugPP now starts a background file watcher automatically after `loadPlugin()` succeeds.

- `HOTPLUGPP_USE_EFSW=ON` enables `efsw` integration when an installed package or fetched source is available.
- `HOTPLUGPP_FETCH_EFSW=ON` allows CMake to fetch `efsw` with `FetchContent` when it is not already installed.
- If `efsw` is unavailable or the fetch probe cannot reach the source, HotPlugPP logs a status message and keeps hot-reload enabled by using the built-in polling watcher instead.
- If the plugin path is invalid or the containing directory cannot be watched, the plugin still loads; only automatic watching is disabled for that load.

To force the polling watcher even when `efsw` is available:

```bash
cmake -S . -B build -DHOTPLUGPP_USE_EFSW=OFF
```

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
    loader.checkAndReload();  // Applies queued watcher events on the caller thread

    return 0;
}
```

See [API](https://github.com/fica99/HotPlugPP/wiki/API) for complete API documentation.

`checkAndReload()` still needs to be called from your main loop. The background watcher only marks the plugin as pending reload, coalesces duplicate change notifications, and leaves the unload/reload work on the caller thread.

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
- **[Hot-Reload Watcher](docs/hot-reload-watcher.md)** - `efsw` integration, polling fallback, and verification steps
- **[Multi-Agent Workflow](docs/multi-agent-workflow.md)** - Autonomous multi-agent development process
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Contribution guidelines

## Examples

The `examples/` directory contains:
- `host_app.cpp` - Host application that applies watcher-queued reloads from the main loop
- `sample_plugin/` - Simple plugin demonstrating basic features
- `math_plugin/` - Complex plugin with state management

Run example with hot-reload:
```bash
# Terminal 1: Build and run the host
cmake --build build --config Release --target build_host_app build_sample_plugin
./build/bin/host_app ./build/bin/libsample_plugin.so

# Terminal 2: Modify and rebuild
# Edit examples/sample_plugin/sample_plugin.cpp
cmake --build build --config Release --target build_sample_plugin
# The watcher will queue one reload and the host loop will apply it on the next check.
```

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

**Code Formatting**: Run `./scripts/format-code.sh` (Linux/macOS) or `scripts\format-code.bat` (Windows) before submitting.

## License

See [LICENSE](LICENSE) file for details.
