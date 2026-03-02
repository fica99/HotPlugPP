**Implementation Plan (Issue #19)**

**Scope**
- Add/finish a non-terminal GUI host sample using Dear ImGui that demonstrates `load`, `unload`, and `check/reload`.
- Integrate GUI sample into CMake behind `HOTPLUGPP_BUILD_GUI_EXAMPLE`.
- Ensure dependency-missing behavior is predictable (clear warning + skip target, no hard failure).
- Document configure/build/run steps for the GUI sample.

**Non-goals**
- No production-grade UI/UX work.
- No multi-backend renderer support in v1 (stick to one minimal backend path).
- No `PluginLoader` API/ABI refactor.

**File-level change list (planned)**
- `examples/gui_host_app/main.cpp`: implement/adjust GUI flow and plugin state rendering (name/version/status).
- `examples/gui_host_app/CMakeLists.txt`: dependency detection, target wiring, copy plugin artifact near executable.
- `examples/CMakeLists.txt`: conditionally include GUI example via feature option.
- `CMakeLists.txt`: define/propagate `HOTPLUGPP_BUILD_GUI_EXAMPLE` option.
- `README.md` (or `docs/*`): add concise build/run instructions and dependency notes.
- Optional compatibility glue if needed by existing checks: `examples/gui_host_app.cpp`.

**Risks**
- ImGui/GLFW/OpenGL availability differs by machine/CI.
  - Mitigation: guard target creation with explicit checks and warning-based skip.
- Multi-config artifact paths (Debug/Release) can break sample-plugin discovery.
  - Mitigation: copy artifact post-build and verify with contract checks.
- Platform-specific library naming/path assumptions.
  - Mitigation: keep extension handling platform-conditional and fallback path logic simple.

**Acceptance checks**
- Configure/build succeeds with default options (GUI OFF).
- Configure/build succeeds with GUI ON + valid deps.
- With GUI ON, `gui_host_app` supports interactive `Load`, `Unload`, `Check/Reload`.
- UI shows plugin state and, when loaded, plugin name/version.
- Existing tests/examples remain green.
- README/docs include GUI configure/build/run instructions.

**DoD commands**
```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake -P scripts/check-format.cmake
```

- Summary of changes: none (planning stage)  
- Files changed: none  
- Validation commands run: none  
- Remaining assumptions/risks: GUI dependencies (`Dear ImGui`, `glfw3`, `OpenGL`) are available in target environments where `HOTPLUGPP_BUILD_GUI_EXAMPLE=ON`; CI coverage for GUI-enabled configuration may need explicit job enablement.

AGENT_STATUS: {"status":"READY","findings":0}