**Architecture/Design Handoff (Issue #19)**

### 1. Target Architecture
1. Keep `hotplugpp::PluginLoader` unchanged (no API/ABI edits).
2. Implement GUI behavior entirely in example layer:
   - `examples/gui_host_app/main.cpp`: runtime/UI state machine.
   - `examples/gui_host_app/CMakeLists.txt`: dependency detection + target wiring + artifact copy.
   - `examples/CMakeLists.txt` + root `CMakeLists.txt`: feature-flag integration (`HOTPLUGPP_BUILD_GUI_EXAMPLE`).
3. Maintain compatibility shim:
   - `examples/gui_host_app.cpp` remains present for source-contract checks and forwards to `gui_host_app/main.cpp`.
4. Documentation remains in `README.md` under GUI section with explicit configure/build/run steps.

### 2. Key Technical Decisions
1. Backend choice: Dear ImGui + GLFW + OpenGL3 only (single minimal backend path).
2. Build gating:
   - `HOTPLUGPP_BUILD_GUI_EXAMPLE` defaults `OFF`.
   - If `HOTPLUGPP_IMGUI_DIR`, `glfw3`, or `OpenGL::GL` is missing, emit clear CMake warning and skip `gui_host_app` target (no configure failure).
3. Plugin discovery:
   - GUI app computes default plugin path from executable directory, with parent-dir fallback for multi-config layouts.
   - `sample_plugin` is copied next to `gui_host_app` post-build.
4. UI action model:
   - Buttons map 1:1 to `loadPlugin`, `unloadPlugin`, `checkAndReload`.
   - Status text must always reflect outcome of last action.
   - Metadata panel reads from `loader.getPlugin()` and shows name/version only when loaded.

### 3. Module Boundaries
1. Core library (`include/`, `src/`):
   - No functional changes.
   - No new public API.
2. Example runtime (`examples/gui_host_app/main.cpp`):
   - Owns window loop, ImGui frame lifecycle, input path buffer, plugin control actions, status rendering.
3. Build integration (`examples/gui_host_app/CMakeLists.txt`, `examples/CMakeLists.txt`, root `CMakeLists.txt`):
   - Owns dependency checks, optional target creation, linking, and plugin artifact colocation.
4. Documentation (`README.md`):
   - Owns user-facing instructions and dependency notes.

### 4. Trade-offs
1. Vendoring ImGui as source files from `HOTPLUGPP_IMGUI_DIR` avoids package-manager assumptions, but requires manual path setup.
2. Warning+skip behavior improves portability/CI resilience, but users may miss GUI target unless they read configure output.
3. Manual “Check/Reload” keeps behavior deterministic and demo-friendly, but is less automated than polling reload loop.

### 5. Integration Constraints
1. Do not break existing tests that assert specific GUI source/doc snippets:
   - `tests/cmake/check_gui_example_source.cmake.in`
   - `tests/cmake/check_gui_example_docs.cmake.in`
2. Preserve `examples/gui_host_app.cpp` containing required textual snippets (or equivalent contract satisfaction).
3. Preserve cross-platform plugin naming handling (`.dll/.so/.dylib`).
4. Ensure build still succeeds with default options (`HOTPLUGPP_BUILD_GUI_EXAMPLE=OFF`).
5. Do not introduce mandatory GUI dependencies into non-GUI builds.

### 6. Risks to Watch
1. Multi-config output path mismatch causing plugin not found at runtime.
2. `glfw3` imported target naming variance (`glfw`, `glfw3`, `glfw3::glfw`).
3. ImGui path correctness (required backend files missing).
4. Runtime reload behavior when plugin rebuild timing causes transient load failures.

### 7. Exact Acceptance Checkpoints for Implementer
1. Configure/build baseline succeeds (GUI off):
   - `cmake -S . -B build`
   - `cmake --build build --config Release --parallel`
2. GUI-enabled configure path is deterministic:
   - With dependencies present and valid `HOTPLUGPP_IMGUI_DIR`, `gui_host_app` target is created.
   - With missing GUI deps, configure does not fail; emits warning and skips target.
3. GUI functional behavior:
   - Buttons `Load`, `Unload`, `Check/Reload` execute respective loader operations.
   - UI shows state (`Loaded/Unloaded`) and metadata (`Name`, `Version`) when loaded.
   - Status message updates for success/failure/no-change/reload events.
4. Artifact availability:
   - `gui_host_app` executable built.
   - `sample_plugin` copied beside executable (for at least Windows multi-config paths covered by current checks).
5. Contract tests remain green:
   - `gui_example_source_contract`
   - `gui_example_docs_contract`
   - `gui_example_artifacts` (when GUI target exists)
6. Full project validation passes:
   - `ctest --test-dir build -C Release --output-on-failure`
   - `cmake -P scripts/check-format.cmake`

- Summary of changes: none (architecture stage)  
- Files changed: none  
- Validation commands run: none  
- Remaining assumptions/risks: GUI dependency availability (`Dear ImGui` sources via `HOTPLUGPP_IMGUI_DIR`, `glfw3`, `OpenGL`) is environment-dependent; GUI artifact checks are currently Windows-oriented and may require extension if Linux/macOS artifact validation is later enforced.

AGENT_STATUS: {"status":"READY","findings":0}