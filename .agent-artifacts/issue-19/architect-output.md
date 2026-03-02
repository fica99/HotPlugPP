**Architecture Handoff: Issue #19 (GUI Host Example)**

**Objective**
Add an optional, non-terminal GUI host sample using Dear ImGui to demonstrate plugin load/unload/hot-reload with the existing `hotplugpp::PluginLoader`, without changing core API/ABI or breaking default builds.

**Key Technical Decisions**
1. Backend choice: Dear ImGui + GLFW + OpenGL (single backend for v1).
2. Build gating: keep GUI sample behind `HOTPLUGPP_BUILD_GUI_EXAMPLE` (default `OFF`).
3. Dependency model: consume ImGui from external source path (`HOTPLUGPP_IMGUI_DIR`) rather than vendoring.
4. Failure behavior: if ImGui/GLFW/OpenGL is missing, emit clear CMake warning and skip `gui_host_app` target instead of failing full configure/build.
5. Runtime plugin integration: use existing `PluginLoader` flow only (`loadPlugin`, `unloadPlugin`, `checkAndReload`, `getPlugin`, `isLoaded`, reload callback); no ABI/API changes.

**Module Boundaries**
1. Root CMake (`CMakeLists.txt`):
- Owns feature option definition and global toggle semantics only.
- Must not add hard GUI dependency to baseline build path.

2. Examples orchestration (`examples/CMakeLists.txt`):
- Conditionally includes GUI sample subdir when GUI option is enabled.
- Keeps existing `sample_plugin`, `math_plugin`, `host_app` behavior unchanged.

3. GUI sample build unit (`examples/gui_host_app/CMakeLists.txt`):
- Validates dependency availability and defines `gui_host_app` target.
- Links `hotplugpp`, GLFW target, OpenGL target.
- Ensures `sample_plugin` artifact is available next to GUI executable (post-build copy).

4. GUI runtime app (`examples/gui_host_app/main.cpp`):
- Owns UI event loop and user interactions.
- Encapsulates plugin-session state (path, status text, loaded metadata projection).
- Calls into `PluginLoader` only; no loader internals.

5. Documentation (`README.md` or docs):
- Owns build/run instructions and dependency expectations for GUI path.

**Integration Constraints**
1. No edits to `include/hotplugpp/*` public interfaces for this issue.
2. No refactor of `src/plugin_loader.cpp` behavior except strictly necessary bugfixes tied to GUI usage.
3. GUI option must remain non-blocking for CI/default local path (`HOTPLUGPP_BUILD_GUI_EXAMPLE=OFF`).
4. CMake must tolerate different GLFW imported target names (`glfw`, `glfw3`, `glfw3::glfw`) to reduce platform packaging variance.
5. Plugin filename/path handling must respect platform extension conventions (`.dll`, `.so`, `.dylib`).

**Trade-offs**
1. External ImGui checkout vs vendoring:
- Pros: smaller repo, faster iteration on dependency pinning outside core.
- Cons: extra configuration step and potential path misconfiguration.

2. Skip-on-missing-deps vs hard failure:
- Pros: preserves main build reliability and onboarding.
- Cons: GUI feature can appear “enabled but absent” unless warning text is explicit.

3. Single backend (GLFW/OpenGL) vs multi-backend:
- Pros: lower complexity, faster delivery.
- Cons: narrower initial platform/windowing coverage.

**Implementation Decomposition for Implementer**
1. Build option and conditional wiring:
- Confirm root option default `OFF`.
- Ensure examples only add GUI subdir when option `ON`.

2. Dependency-resilient GUI CMake target:
- Validate `HOTPLUGPP_IMGUI_DIR` and required ImGui backend files.
- `find_package(glfw3 QUIET)` + target resolution fallback.
- `find_package(OpenGL QUIET)` and require `OpenGL::GL`.
- Create `gui_host_app` target and link requirements.

3. Runtime GUI behavior:
- Add controls for `Load`, `Unload`, `Check/Reload`.
- Display state: loaded/unloaded, plugin name/version (and optional description/status).
- Add clear user-visible status messages for success/failure/no-op cases.

4. Artifact locality:
- Add dependency/copy rule so `sample_plugin` lands beside `gui_host_app` binary for straightforward launch.

5. Docs:
- Add exact configure/build/run commands.
- Document dependency prerequisites and “skipped target with warning” behavior.

**Risks**
1. CMake target mismatch for GLFW package across systems.
2. ImGui path valid but incomplete checkout (missing backend sources).
3. Multi-config generators (Visual Studio) causing plugin copy path mismatch.
4. OpenGL/GLFW unavailable in some CI jobs; must not fail non-GUI pipeline.
5. Runtime plugin path defaults differing across OS/build layouts.

**Exact Acceptance Checkpoints (Implementer must verify)**
1. Configure baseline path:
- `cmake -S . -B build`
- Expected: succeeds with default GUI option off; existing targets unaffected.

2. Configure GUI path with valid deps:
- `cmake -S . -B build -DHOTPLUGPP_BUILD_GUI_EXAMPLE=ON -DHOTPLUGPP_IMGUI_DIR=<valid_imgui_path>`
- Expected: `gui_host_app` target generated.

3. Build:
- `cmake --build build --config Release --parallel`
- Expected: `gui_host_app` and `sample_plugin` build successfully.

4. Runtime behavior:
- Run GUI executable.
- Expected:
  - `Load` loads plugin from path.
  - `Unload` unloads plugin.
  - `Check/Reload` triggers reload check.
  - UI shows loaded state and plugin name/version when loaded.

5. Degraded dependency path:
- Configure GUI `ON` with missing/invalid deps.
- Expected: clear warning(s), `gui_host_app` skipped, overall configure/build still succeeds for remaining project.

6. Regression safety:
- `ctest --test-dir build -C Release --output-on-failure`
- Expected: existing tests remain passing.
- `cmake -P scripts/check-format.cmake`
- Expected: format check passes.

7. Documentation:
- README/docs includes GUI-specific prerequisites and exact commands.

**Handoff Status**
AGENT_STATUS: {"status":"READY","findings":0}

Summary of changes: none (architecture stage)  
Files changed: none  
Validation commands run: none  
Remaining assumptions/risks: ImGui is provided as external source via `HOTPLUGPP_IMGUI_DIR`; at least one local/CI environment has functional GLFW/OpenGL for runtime verification; GUI feature remains optional and must not block default pipelines if dependencies are unavailable.