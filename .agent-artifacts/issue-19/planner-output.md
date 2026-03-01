**Implementation Plan**
1. CMake gating and graceful fallback.
- Add `HOTPLUGPP_BUILD_GUI_EXAMPLE` option (default `OFF`) in [CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\CMakeLists.txt).
- Wire the option into examples build flow in [examples/CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\examples\CMakeLists.txt).
- If GUI deps are missing, print a clear CMake warning and skip only the GUI target (no hard failure, no impact on existing examples/tests).

2. Add new GUI host sample.
- Add new target folder and source: [examples/gui_host_app/main.cpp](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\examples\gui_host_app\main.cpp) and [examples/gui_host_app/CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\examples\gui_host_app\CMakeLists.txt).
- Implement Dear ImGui + minimal backend loop (GLFW/OpenGL) with actions: `Load`, `Unload`, `Check/Reload`.
- Reuse existing `hotplugpp::PluginLoader` API only (no ABI/public API changes).
- Display plugin state + name/version when loaded, plus last action/error text.

3. Documentation update.
- Add build/run notes for GUI sample in [README.md](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\README.md) (or docs file if preferred), including option enablement and expected dependency behavior.

**Scope**
- New non-terminal interactive GUI host sample.
- CMake integration under feature flag.
- Short user-facing build/run documentation.
- Preserve existing CLI sample behavior and current plugin ABI/API.

**Non-Goals**
- Production-grade UI/UX polish.
- Multiple rendering backends in v1.
- Refactor of `PluginLoader` or core library architecture.
- Mandatory GUI build in all CI jobs.

**Planned File-Level Change List**
- Modify [CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\CMakeLists.txt)
- Modify [examples/CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\examples\CMakeLists.txt)
- Add [examples/gui_host_app/CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\examples\gui_host_app\CMakeLists.txt)
- Add [examples/gui_host_app/main.cpp](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\examples\gui_host_app\main.cpp)
- Modify [README.md](C:\Users\ficac\Desktop\projects\my\HotPlugPP-planner-issue-19\README.md)

**Acceptance Checks**
- `HOTPLUGPP_BUILD_GUI_EXAMPLE=OFF`: project config/build/tests unchanged.
- `HOTPLUGPP_BUILD_GUI_EXAMPLE=ON` with deps present: GUI sample target builds.
- GUI runtime supports interactive `load`, `unload`, `check/reload`.
- UI shows loaded state + plugin name/version.
- Existing examples/tests remain green.
- README/docs include clear build and launch instructions.

**DoD Commands**
- `cmake -S . -B build`
- `cmake --build build --config Release --parallel`
- `ctest --test-dir build -C Release --output-on-failure`
- `cmake -P scripts/check-format.cmake`
- Additional GUI verification build (when deps installed): `cmake -S . -B build-gui -DHOTPLUGPP_BUILD_GUI_EXAMPLE=ON` then `cmake --build build-gui --config Release --parallel`

- Summary of changes: none (planning stage)
- Files changed: none
- Validation commands run: none
- Remaining assumptions/risks: Dear ImGui dependency sourcing strategy is not yet fixed (vendored vs system); headless CI may not execute GUI runtime, so verification may stay build-level in CI and runtime-level locally; plugin path/extension handling must be OS-aware in docs and sample defaults.