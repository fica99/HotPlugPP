# Changelog

## Issue #18 - Watcher-based plugin hot-reload

- Added automatic plugin file watching in `PluginLoader`, with `efsw` integration when available.
- Added a built-in polling watcher fallback so hot-reload still works when `efsw` is unavailable or cannot be fetched.
- Kept reload execution on the caller thread: watcher callbacks only queue a reload, and `checkAndReload()` applies it.
- Added debounce handling so rapid duplicate file-change notifications coalesce into a single reload.
- Added tests for file-change-triggered reloads, duplicate change coalescing, and fallback behavior when watcher setup rejects a path.
- Updated build behavior so examples and tests are excluded from the default build and must be built explicitly by target.
- Consolidated test execution under `hotplugpp_tests`, with automatic fallback to the bundled test shim when GoogleTest is unavailable.
