**Architecture/Design Handoff (Issue #19)**

**Architecture Goal**
Add an optional GUI host example that demonstrates `load`, `unload`, and `check/reload` using existing `hotplugpp::PluginLoader`, with zero impact on core library ABI/API and existing non-GUI builds.

**Key Technical Decisions**
1. Keep core untouched.
- No changes in [`include/hotplugpp/plugin_loader.hpp`](C:\Users\ficac\Desktop\projects\my\HotPlugPP-architect-issue-19\include\hotplugpp\plugin_loader.hpp) or [`src/plugin_loader.cpp`](C:\Users\ficac\Desktop\projects\my\HotPlugPP-architect-issue-19\src\plugin_loader.cpp).
- GUI sample is consumer-only.

2. Feature-gated build integration.
- Add root option `HOTPLUGPP_BUILD_GUI_EXAMPLE` default `OFF` in [`CMakeLists.txt`](C:\Users\ficac\Desktop\projects\my\HotPlugPP-architect-issue-19\CMakeLists.txt).
- Gate GUI target wiring in [`examples/CMakeLists.txt`](C:\Users\ficac\Desktop\projects\my\HotPlugPP-architect-issue-19\examples\CMakeLists.txt).

3. Predictable dependency fallback.
- If GUI deps are missing, emit `message(WARNING ...)` and skip only GUI target.
- Never fail configure/build for main library, tests, or existing examples when GUI option is on but deps are absent.

4. Backend scope.
- First version supports one backend only: Dear ImGui + GLFW + OpenGL.
- No multi-backend abstraction in this issue.

5. Explicit runtime state model in GUI app.
- State fields: `is_loaded`, `plugin_path`, `plugin_name`, `plugin_version`, `last_action`, `last_error`.
- UI must reflect that `checkAndReload()` can unload and fail reload (loader may end unloaded).

**Module Boundaries**
1. Build system boundary.
- Root CMake: option declaration only.
- Examples CMake: GUI dependency detection and target registration.

2. GUI sample boundary.
- New folder [`examples/gui_host_app/`](C:\Users\ficac\Desktop\projects\my\HotPlugPP-architect-issue-19\examples\gui_host_app).
- [`examples/gui_host_app/main.cpp`](C:\Users\ficac\Desktop\projects\my\HotPlugPP-architect-issue-19\examples\gui_host_app\main.cpp): event/render loop + plugin control panel.
- [`examples/gui_host_app/CMakeLists.txt`](C:\Users\ficac\Desktop\projects\my\HotPlugPP-architect-issue-19\examples\gui_host_app\CMakeLists.txt): local target definition and linking.

3. Documentation boundary.
- Update run/build instructions in [`README.md`](C:\Users\ficac\Desktop\projects\my\HotPlugPP-architect-issue-19\README.md) only; keep docs short and operational.

**Trade-offs**
1. Optional GUI deps vs always-on demo.
- Chosen: optional; preserves portability and CI stability.
- Cost: GUI sample may be skipped on machines without deps.

2. Minimal single-file GUI app vs reusable GUI framework.
- Chosen: minimal app; lower complexity for demo.
- Cost: less reuse if future GUI examples appear.

3. Manual plugin path input vs plugin discovery UX.
- Chosen: manual input field/button; simplest reliable cross-platform behavior.
- Cost: less user-friendly than file picker.

**Integration Constraints**
1. Do not modify public API/ABI or plugin factory contract (`createPlugin`/`destroyPlugin`).
2. Do not alter behavior of existing targets: `host_app`, `sample_plugin`, `math_plugin`, tests.
3. Keep GUI target isolated so formatting/tests/build checks remain green with option `OFF`.
4. Ensure plugin path examples in docs are platform-specific (`.dll` / `.so` / `.dylib`).
5. Preserve current output-directory assumptions (`build/bin`) used by examples.

**Implementation Risks**
1. Reload failure semantics.
- Current loader unloads before reload; on reload failure plugin stays unloaded.
- Mitigation: UI must display this clearly and not assume old instance persists.

2. Dependency detection variance across OS/toolchains.
- Mitigation: use quiet detection + explicit warning text with what was missing.

3. Headless CI limitations.
- Mitigation: CI validation remains build-level for GUI target; runtime verification done locally.

4. File locking/timestamp behavior during rebuild (especially Windows).
- Mitigation: surface last action/error in UI so user can retry and diagnose.

**Acceptance Checkpoints for Implementer (Exact)**
1. Configure/build baseline unchanged:
- `cmake -S . -B build`
- `cmake --build build --config Release --parallel`
- `ctest --test-dir build -C Release --output-on-failure`
- `cmake -P scripts/check-format.cmake`

2. GUI option OFF behavior:
- `cmake -S . -B build-off -DHOTPLUGPP_BUILD_GUI_EXAMPLE=OFF`
- Build succeeds; no GUI target required; existing targets still build.

3. GUI option ON, deps missing:
- `cmake -S . -B build-gui-on -DHOTPLUGPP_BUILD_GUI_EXAMPLE=ON`
- Configure succeeds with clear warning; GUI target skipped only.

4. GUI option ON, deps present:
- Configure/build includes GUI target successfully.
- Launch app and verify interactive buttons:
  - `Load` loads plugin and displays name/version/status.
  - `Unload` unloads and status reflects unloaded.
  - `Check/Reload` attempts reload and status reflects success/failure correctly.

5. Documentation:
- README includes enable flag, dependency expectations, and launch command for GUI sample.

AGENT_STATUS: {"status":"READY","findings":0}

- Summary of changes: none (architecture stage)
- Files changed: none
- Validation commands run: none
- Remaining assumptions/risks: Dear ImGui source strategy (vendored vs local dependency) must be fixed during implementation; GUI runtime verification is local if CI is headless; plugin path examples must remain OS-specific and consistent with actual output names.