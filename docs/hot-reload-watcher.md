# Hot-Reload Watcher

Issue #18 adds automatic plugin file watching to `PluginLoader`.

## Build-time behavior

HotPlugPP tries to use `efsw` first:

- `HOTPLUGPP_USE_EFSW=ON` enables watcher integration.
- `HOTPLUGPP_FETCH_EFSW=ON` lets CMake fetch `efsw` with `FetchContent` when no package is already installed.

If `efsw` is unavailable or the source probe cannot reach GitHub, configuration continues and HotPlugPP falls back to its built-in polling watcher.

Examples:

```bash
# Default behavior: use efsw when available, otherwise poll
cmake -S . -B build

# Force polling only
cmake -S . -B build -DHOTPLUGPP_USE_EFSW=OFF

# Disable network fetches and rely only on an installed efsw package
cmake -S . -B build -DHOTPLUGPP_FETCH_EFSW=OFF
```

## Runtime behavior

- `loadPlugin()` starts a background watcher for the loaded plugin path when the path points to a `.dll`, `.so`, or `.dylib`.
- Watch registration failures are non-fatal. The plugin still loads, and `checkAndReload()` continues to work by checking the file modification time directly.
- The watcher never unloads or reloads a plugin from the callback thread. It only marks a reload as pending.
- `checkAndReload()` must still be called from the host loop to apply the reload safely.
- Rapid duplicate file notifications are debounced so one rebuild triggers one reload.

## Why rebuilds work while the plugin is loaded (shadow copy)

On Windows the OS holds an exclusive lock on a loaded `.dll`, so loading the
build-output file directly would block the next rebuild from overwriting it and
hot-reload would never fire. To avoid this, `PluginLoader` loads a **shadow copy**:

- On load it copies the library to a uniquely named sibling file in the same
  directory (`.hotplugpp-shadow-<name>-<pid>-<n>.<ext>`) and loads that copy
  (`LoadLibraryW` on Windows, `dlopen` on POSIX). The original build-output file is
  never opened by the loader, so it stays writable for rebuilds.
- The shadow copy lives in the *same* directory as the original (not a temp dir),
  so dependency resolution is unchanged on Windows and `$ORIGIN`/`@loader_path`
  dependencies keep resolving on POSIX.
- The shadow filename differs from the watched original, so the watcher ignores
  shadow create/delete events — there is no reload loop.
- Shadow copies are deleted on unload, on reload, and on a failed load. A rare
  crash may leave one behind; they are gitignored and harmless.
- If the plugin directory is not writable, the loader falls back to loading the
  original file directly. Loading still succeeds, but on Windows the original is
  then locked, so hot-reload may not work until the plugin is unloaded.

## Local verification

```bash
cmake -S . -B build
cmake --build build --config Release --target build_host_app build_sample_plugin
./build/bin/host_app ./build/bin/libsample_plugin.so
```

Then:

1. Edit `examples/sample_plugin/sample_plugin.cpp`.
2. Rebuild `sample_plugin`.
3. Watch the host process log `Plugin file modified, reloading...`.

If you intentionally pass an invalid plugin path or a non-library extension, HotPlugPP logs that the watcher could not be started and continues without automatic watching for that load.
