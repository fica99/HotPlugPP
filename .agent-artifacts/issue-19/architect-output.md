**Architecture Handoff: Issue #19 (GUI Host Sample with ImGui)**

## 1. Architecture intent
Add an optional, non-terminal GUI host sample that demonstrates plugin lifecycle actions (`load`, `unload`, manual `check/reload`) using existing `hotplugpp::PluginLoader` only, with predictable CMake degradation when GUI deps are unavailable.

## 2. Key technical decisions

1. **Optional feature gate at configure-time**
- Use `HOTPLUGPP_BUILD_GUI_EXAMPLE` (default `OFF`) at top-level.
- Decision rationale: preserves current default builds and CI behavior without GUI dependencies.

2. **Dependency model: vendored ImGui source path + system GLFW/OpenGL**
- Require `HOTPLUGPP_IMGUI_DIR` (path containing `imgui.h`, core `.cpp` files, and backend sources).
- Resolve GLFW via `find_package(glfw3)` and accept target variants: `glfw`, `glfw3`, or `glfw3::glfw`.
- Require `OpenGL::GL`.
- Trade-off: lowest integration complexity; avoids adding package managers/fetch logic in v1.

3. **Graceful degradation instead of hard CMake failure**
- If ImGui dir is unset/invalid or GLFW/OpenGL missing, emit `message(WARNING ...)` and skip `gui_host_app` target.
- Trade-off: feature may silently be absent if warnings are ignored, but core build remains stable and matches issue requirements.

4. **No PluginLoader API/ABI change**
- GUI interacts strictly via:
  - `loadPlugin(path)`
  - `unloadPlugin()`
  - `checkAndReload()`
  - `isLoaded()`
  - `getPlugin()`
  - `setReloadCallback(...)`
- Constraint: avoid touching `include/hotplugpp/*.hpp` public contracts.

5. **Runtime artifact co-location**
- `gui_host_app` depends on `sample_plugin`.
- Post-build copy of `sample_plugin` next to `gui_host_app` executable (`copy_if_different`).
- Rationale: removes manual path friction for demos and keeps runtime deterministic.

6. **Compatibility shim for source-contract tests**
- Keep `examples/gui_host_app.cpp` as forwarding shim including `examples/gui_host_app/main.cpp`.
- Constraint: preserve required snippets used by `gui_example_source_contract` (including exact `"Check/Reload"` token currently enforced there).

## 3. Module boundaries

1. **Top-level CMake (`CMakeLists.txt`)**
- Owns global option definition and subdirectory inclusion.
- Must not embed GUI dependency resolution logic here beyond option declaration.

2. **Examples aggregator (`examples/CMakeLists.txt`)**
- Owns conditional `add_subdirectory(gui_host_app)` under `HOTPLUGPP_BUILD_GUI_EXAMPLE`.

3. **GUI module (`examples/gui_host_app/CMakeLists.txt`)**
- Sole owner of ImGui/GLFW/OpenGL discovery and skip logic.
- Builds `gui_host_app`, links `hotplugpp`, copies plugin runtime artifact.

4. **GUI behavior (`examples/gui_host_app/main.cpp`)**
- Owns application loop, UI controls, status text, and plugin metadata display.
- Must not implement plugin internals or alter loader semantics.

5. **Contract compatibility file (`examples/gui_host_app.cpp`)**
- Exists to satisfy test/source path contracts; no separate business logic.

6. **Docs (`README.md`)**
- Must document option, dependencies, build/run path, and expected skip behavior.

## 4. Integration constraints for Implementer

1. Keep target name exactly `gui_host_app`.
2. Keep CMake option name exactly `HOTPLUGPP_BUILD_GUI_EXAMPLE`.
3. Do not remove or rename `examples/gui_host_app.cpp` unless tests are updated in same change.
4. Preserve/align source-contract snippets checked in `tests/cmake/check_gui_example_source.cmake.in`:
- `loadPlugin(`
- `unloadPlugin(`
- `checkAndReload(`
- `getName()`
- `getVersion().toString()`
- `"Load"`
- `"Unload"`
- `"Check/Reload"` (exact string as current contract)
5. Preserve README snippets required by `gui_example_docs_contract`:
- `## GUI Sample`
- `HOTPLUGPP_BUILD_GUI_EXAMPLE`
- `gui_host_app`
- `cmake -S . -B build -DHOTPLUGPP_BUILD_GUI_EXAMPLE=ON`
6. Ensure artifact expectations for Windows contract test remain true when GUI target exists:
- `build/bin/.../gui_host_app.exe`
- `build/bin/.../sample_plugin.dll`
7. No edits to `include/hotplugpp/i_plugin.hpp` or `include/hotplugpp/plugin_loader.hpp` for this issue.

## 5. Risks and mitigations

1. **Brittle string-based source contract**
- Risk: UI label `"Check / Reload"` in main UI may diverge from tested `"Check/Reload"`.
- Mitigation: keep compatibility snippet in forwarding source or harmonize both source and test intentionally.

2. **Generator/config-specific output path drift**
- Risk: multi-config output dirs may break artifact checks.
- Mitigation: keep runtime output rooted at `${CMAKE_BINARY_DIR}/bin` and verify all configs.

3. **Dependency target name variance**
- Risk: GLFW target mismatch across package providers.
- Mitigation: keep multi-target fallback logic (`glfw`, `glfw3`, `glfw3::glfw`).

4. **Runtime reload UX ambiguity**
- Risk: `checkAndReload()` returns `false` for both “no change” and “reload failed after unload”.
- Mitigation: UI status logic must explicitly check loaded-state transitions to distinguish failure/no-change.

5. **Platform scope creep**
- Risk: attempts to support multiple render backends now increase complexity.
- Mitigation: explicitly constrain to GLFW + OpenGL only for v1.

## 6. Acceptance checkpoints for Implementer (exact)

1. **GUI OFF baseline**
- Configure/build with default options.
- Expected: project builds; no GUI dependency required; tests unaffected.

2. **GUI ON with valid deps**
- Configure with:
  - `-DHOTPLUGPP_BUILD_GUI_EXAMPLE=ON`
  - `-DHOTPLUGPP_IMGUI_DIR=<valid_imgui_path>`
- Expected: `gui_host_app` target is generated and builds.

3. **GUI ON missing deps**
- Same as above but missing/invalid deps.
- Expected: clear CMake warning; `gui_host_app` skipped; configure/build still succeeds for core targets.

4. **Source/docs contract tests**
- `ctest` includes and passes:
  - `gui_example_source_contract`
  - `gui_example_docs_contract`
- `gui_example_artifacts` runs only when `gui_host_app` target exists and must pass then.

5. **Runtime smoke**
- Launch GUI app.
- Perform `Load`, `Unload`, `Check / Reload`.
- Expected UI shows:
  - loaded/unloaded state
  - plugin name/version when loaded
  - status message updates for action results.

6. **Project-wide DoD commands**
- `cmake -S . -B build`
- `cmake --build build --config Release --parallel`
- `ctest --test-dir build -C Release --output-on-failure`
- `cmake -P scripts/check-format.cmake`

- Summary of changes: none (architecture stage)
- Files changed: none
- Validation commands run: none
- Remaining assumptions/risks: Implementer has access to a valid Dear ImGui checkout path for GUI-on validation; GLFW/OpenGL discovery remains environment-dependent; current source-contract check for `"Check/Reload"` is intentionally brittle and must be preserved or updated in lockstep.