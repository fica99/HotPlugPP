**Implementation Plan (Issue #19)**

**Scope**
1. Add/complete a non-terminal GUI host sample using Dear ImGui that demonstrates `load`, `unload`, and manual `check/reload`.
2. Keep plugin interaction on existing `PluginLoader` API only (no ABI/API changes).
3. Gate GUI sample behind `HOTPLUGPP_BUILD_GUI_EXAMPLE` and make dependency failures degrade predictably (clear CMake warning + skipped target).
4. Integrate GUI sample into examples build flow and ensure `sample_plugin` is available at runtime.
5. Document build/run steps in `README.md` (or docs), including required GUI dependencies.

**Non-Goals**
1. Production-grade UI/UX styling or advanced GUI architecture.
2. Multi-backend rendering support beyond the first minimal backend.
3. Refactoring `PluginLoader` core behavior or public API.
4. Expanding plugin feature scope beyond status/name/version visibility and basic control flow.

**File-Level Change List**
1. `CMakeLists.txt`
   - Define/confirm `HOTPLUGPP_BUILD_GUI_EXAMPLE` option and examples integration behavior.
2. `examples/CMakeLists.txt`
   - Conditionally include GUI sample subdirectory.
3. `examples/gui_host_app/CMakeLists.txt`
   - Resolve ImGui/GLFW/OpenGL dependencies.
   - Add graceful skip/warning path when deps are missing.
   - Build `gui_host_app`, link `hotplugpp`, copy plugin artifact near executable.
4. `examples/gui_host_app/main.cpp`
   - Implement interactive GUI host: plugin path input, `Load`, `Unload`, `Check / Reload`, state + name/version/status rendering.
5. `examples/gui_host_app.cpp` (if used as compatibility/contract shim)
   - Keep source-contract compatibility if tests depend on this path.
6. `README.md` (or `docs/...`)
   - Add concise GUI sample build/run instructions and option/dependency notes.
7. `tests/cmake/check_gui_example_*.cmake.in` and `tests/CMakeLists.txt` (Tester phase if needed)
   - Ensure contract checks match final source/doc paths and expected labels/snippets.

**Primary Risks**
1. GUI dependency discovery varies across environments (`glfw` target names, OpenGL packages).
2. Runtime plugin path/copy behavior differs by generator/config (`Debug/Release`, single vs multi-config).
3. Contract tests can become brittle if UI labels/source locations drift.
4. Platform coverage may be limited initially; define at least one validated platform baseline.

**Acceptance Checks**
1. Configure with GUI OFF: project builds normally, no GUI dependency requirement.
2. Configure with GUI ON + valid deps: `gui_host_app` target is generated and builds.
3. Configure with GUI ON + missing deps: CMake emits clear warning and skips GUI target without failing core build.
4. Runtime smoke: GUI can load/unload/reload `sample_plugin`; UI shows loaded state and plugin name/version.
5. Existing examples/tests remain passing (or explicitly gated where external deps are absent).
6. README/docs contain reproducible GUI build/run instructions.

**DoD Commands**
```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake -P scripts/check-format.cmake
```
Additional GUI-on validation:
```powershell
cmake -S . -B build-gui -DHOTPLUGPP_BUILD_GUI_EXAMPLE=ON -DHOTPLUGPP_IMGUI_DIR=<path-to-imgui>
cmake --build build-gui --config Release --parallel
ctest --test-dir build-gui -C Release --output-on-failure
```

- Summary of changes: none (planning stage)
- Files changed: none
- Validation commands run: none
- Remaining assumptions/risks: ImGui checkout path is provided and valid; `glfw3`/OpenGL are discoverable in CI or local baseline; at least one target platform is designated as required GUI validation baseline.

AGENT_STATUS: {"status":"READY","findings":0}