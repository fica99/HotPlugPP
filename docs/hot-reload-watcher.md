# Hot-Reload Watcher

Issue #18 adds automatic plugin file watching to `PluginLoader`.

## Build-time behavior

HotPlugPP tries to use `efsw` first:

- `HOTPLUGPP_USE_EFSW=ON` enables watcher integration.
- `HOTPLUGPP_FETCH_EFSW=ON` lets CMake fetch `efsw` with `FetchContent` when no package is already installed.
- `HOTPLUGPP_USE_BUNDLED_EFSW_STUB=ON` enables the bundled `efsw`-compatible shim if the real dependency is unavailable.

If `efsw` is unavailable or the source probe cannot reach GitHub, configuration continues and HotPlugPP falls back to its built-in polling watcher.

Examples:

```bash
# Default behavior: use efsw when available, otherwise poll
cmake -S . -B build

# Force polling only
cmake -S . -B build -DHOTPLUGPP_USE_EFSW=OFF

# Disable network fetches and rely only on an installed efsw package
cmake -S . -B build -DHOTPLUGPP_FETCH_EFSW=OFF

# Disable both efsw and the bundled shim to use only the built-in polling watcher
cmake -S . -B build -DHOTPLUGPP_USE_EFSW=ON -DHOTPLUGPP_USE_BUNDLED_EFSW_STUB=OFF -DHOTPLUGPP_FETCH_EFSW=OFF
```

## Runtime behavior

- `loadPlugin()` starts a background watcher for the loaded plugin path when the path points to a `.dll`, `.so`, or `.dylib`.
- Watch registration failures are non-fatal. The plugin still loads, and `checkAndReload()` continues to work by checking the file modification time directly.
- The watcher never unloads or reloads a plugin from the callback thread. It only marks a reload as pending.
- `checkAndReload()` must still be called from the host loop to apply the reload safely.
- Rapid duplicate file notifications are debounced so one rebuild triggers one reload.

## Local verification

```bash
cmake -S . -B build
cmake --build build --config Release --target host_app sample_plugin
./build/bin/host_app ./build/bin/libsample_plugin.so
```

Then:

1. Edit `examples/sample_plugin/sample_plugin.cpp`.
2. Rebuild `sample_plugin`.
3. Watch the host process log `Plugin file modified, reloading...`.

If you intentionally pass an invalid plugin path or a non-library extension, HotPlugPP logs that the watcher could not be started and continues without automatic watching for that load.
