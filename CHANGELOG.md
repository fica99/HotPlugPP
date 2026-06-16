# Changelog

## Unreleased

- Removed platform dynamic-loader headers from `plugin_loader.hpp`; Windows consumers no longer inherit `windows.h` macros from this public header.
- Moved plugin loader native handles and factory-function state behind a private PImpl with RAII library unloading.
- Added a public-header hygiene compile test for Win32 `min`/`max` macro leakage.
- Fixed hot-reload on Windows: `PluginLoader` now loads a co-located **shadow copy**
  of the plugin library instead of the build-output file directly, so the OS lock on
  a loaded `.dll` no longer prevents a rebuild from overwriting the original. Applied
  on all platforms.
- The shadow copy is created next to the original
  (`.hotplugpp-shadow-<name>-<pid>-<n>.<ext>`) and removed on unload, reload, and
  failed load; co-locating it preserves dependency resolution on Windows and
  `$ORIGIN`/`@loader_path` on POSIX. Falls back to a direct load when the plugin
  directory is not writable.
- Added regression tests: overwriting a loaded library now reloads, shadow copies
  are removed after unload, and a failed load leaves none behind.

## Issue #18 - Watcher-based plugin hot-reload

- Added automatic plugin file watching in `PluginLoader`, with `efsw` integration when available.
- Added a built-in polling watcher fallback so hot-reload still works when `efsw` is unavailable or cannot be fetched.
- Kept reload execution on the caller thread: watcher callbacks only queue a reload, and `checkAndReload()` applies it.
- Added debounce handling so rapid duplicate file-change notifications coalesce into a single reload.
- Added tests for file-change-triggered reloads, duplicate change coalescing, and fallback behavior when watcher setup rejects a path.
- Updated build behavior so examples and tests are excluded from the default build and must be built explicitly by target.
- Consolidated test execution under `hotplugpp_tests`, with automatic fallback to the bundled test shim when GoogleTest is unavailable.
