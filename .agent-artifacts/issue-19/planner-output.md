**Implementation Plan (Issue #19)**

**Scope**
1. Add an optional non-terminal GUI host example using Dear ImGui (GLFW + OpenGL backend) under `examples/gui_host_app/`.
2. Implement interactive controls for `load`, `unload`, and `check/reload` using existing `hotplugpp::PluginLoader`.
3. Display plugin runtime info in UI: load state, name, version, and status text.
4. Integrate the GUI example into examples build behind `HOTPLUGPP_BUILD_GUI_EXAMPLE`.
5. Document configure/build/run steps and dependency behavior when GUI deps are missing.

**Non-Goals**
1. No production-grade UI/UX polish.
2. No support for multiple renderer/window backends in v1.
3. No ABI or API changes to plugin contracts or `PluginLoader`.
4. No refactor of core loading/hot-reload internals.

**File-Level Change List**
1. `CMakeLists.txt`: define/propagate `HOTPLUGPP_BUILD_GUI_EXAMPLE` option (default OFF) and keep core build unaffected.
2. `examples/CMakeLists.txt`: conditionally include GUI sample subdirectory.
3. `examples/gui_host_app/CMakeLists.txt`: create `gui_host_app` target, validate ImGui/GLFW/OpenGL deps, emit clear warnings and skip target if unavailable, copy `sample_plugin` artifact near executable.
4. `examples/gui_host_app/main.cpp`: implement ImGui app loop + plugin controls + plugin metadata/status rendering.
5. `README.md` (or docs page): add GUI sample build/run instructions and dependency notes.
6. Optional validation-only tests (if missing): `tests/cmake/check_gui_example_*.cmake.in` hooks for source/docs/artifact presence.

**Risks**
1. Dependency discovery differences across platforms (`glfw`, `glfw3`, `glfw3::glfw`) can break target linking.
2. ImGui checkout path may be invalid or incomplete; must fail predictably with non-fatal warning.
3. Runtime plugin path resolution may vary by generator/config; copy rules must place plugin beside GUI executable.
4. OpenGL/GLFW availability in CI may be limited; GUI target must be optional and non-blocking for default pipeline.

**Acceptance Checks**
1. Configure with GUI ON and valid `HOTPLUGPP_IMGUI_DIR`; confirm `gui_host_app` target is generated.
2. Build succeeds and produces GUI host + sample plugin artifacts.
3. Run `gui_host_app`; verify buttons perform `load`, `unload`, `check/reload`.
4. Confirm UI shows plugin state, name, and version when loaded.
5. Configure with missing GUI deps/path; confirm clear warning and graceful skip without breaking other targets/tests.
6. Confirm README/docs includes exact configure/build/run instructions.

**DoD Commands**
```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake -P scripts/check-format.cmake
```

Summary of changes: none (planning stage)  
Files changed: none  
Validation commands run: none  
Remaining assumptions/risks: ImGui is consumed from an external source tree (`HOTPLUGPP_IMGUI_DIR`), not vendored; at least one local/CI environment has GLFW+OpenGL available for functional GUI execution; optional GUI build path must remain isolated so default non-GUI pipeline stays green.

AGENT_STATUS: {"status":"READY","findings":0}