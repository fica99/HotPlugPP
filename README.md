# HotPlugPP

A lightweight, cross-platform plugin system in modern C++ with support for runtime dynamic loading and hot-reloading of shared libraries.

## Features

- 🔌 **Dynamic Loading**: Load and unload plugins at runtime
- 🔥 **Watcher-Based Hot-Reloading**: Queue reloads from a background watcher and apply them on your host loop
- 🌐 **Cross-Platform**: Works on Windows (.dll), Linux (.so), and macOS (.dylib)
- 🎯 **Clean Interface**: Simple, intuitive plugin API
- **Header Hygiene**: Public headers avoid platform dynamic-loader headers such as `windows.h`
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

HotPlugPP starts a background file watcher automatically after `loadPlugin()` succeeds.

- `HOTPLUGPP_USE_EFSW=ON` enables `efsw` integration when an installed package or fetched source is available.
- `HOTPLUGPP_FETCH_EFSW=ON` allows CMake to fetch `efsw` with `FetchContent` when it is not already installed.
- If `efsw` is unavailable, HotPlugPP logs a status message and keeps hot-reload enabled by using the built-in polling watcher instead.
- If the plugin path is invalid or the containing directory cannot be watched, the plugin still loads; only automatic watching is disabled for that load.

To force the polling watcher even when `efsw` is available:

```bash
cmake -S . -B build -DHOTPLUGPP_USE_EFSW=OFF
```

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `HOTPLUGPP_USE_EFSW` | `ON` | Enable efsw file watcher integration |
| `HOTPLUGPP_FETCH_EFSW` | `ON` | Fetch efsw via FetchContent if not installed |
| `HOTPLUGPP_BUILD_TESTS` | `ON` | Configure the test suite (built explicitly via `build_hotplugpp_tests`) |
| `HOTPLUGPP_FETCH_GOOGLETEST` | `ON` | Fetch GoogleTest via FetchContent if not installed; falls back to bundled shim when off |

Offline / air-gapped builds:
```bash
cmake -S . -B build -DHOTPLUGPP_FETCH_EFSW=OFF -DHOTPLUGPP_FETCH_GOOGLETEST=OFF
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

    // Optional: receive a notification after each successful reload.
    loader.setReloadCallback([]() {
        // Called on the host thread, after the new plugin has been loaded.
        // Re-fetch your plugin pointer here if you cache it.
    });

    if (!loader.loadPlugin("./libmy_plugin.so")) {
        return 1;
    }

    while (running) {
        // Apply any watcher-queued reload on the host thread.
        auto result = loader.checkAndReload();
        if (result == hotplugpp::PluginLoader::ReloadResult::ReloadFailed) {
            // Handle reload failure (plugin is now unloaded).
        }

        auto* plugin = loader.getPlugin();
        if (plugin) {
            plugin->onUpdate(deltaTime);
        }
    }
    return 0;
}
```

See [API](https://github.com/fica99/HotPlugPP/wiki/API) for complete API documentation.

`checkAndReload()` must be called from your host thread. The background watcher only marks the plugin as pending reload and coalesces duplicate change events; all unload/reload work happens on the caller thread when `checkAndReload()` is invoked.

### Thread Safety

- `loadPlugin()`, `unloadPlugin()`, `checkAndReload()`, `getPlugin()` — call from **one thread only** (your host/main thread).
- The background watcher thread runs internally and communicates exclusively via atomic flags; it never touches the plugin instance directly.

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
