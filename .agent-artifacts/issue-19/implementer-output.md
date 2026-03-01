Implemented Issue #19 scope in this worktree and fixed a build-order regression blocking required validation.

**Files changed**
- [examples/CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-implementer-issue-19\examples\CMakeLists.txt)
- [examples/gui_host_app/CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-implementer-issue-19\examples\gui_host_app\CMakeLists.txt)

**What changed**
- Removed Visual Studio `VS_GLOBAL_BuildProjectReferences false` overrides for `host_app` and `gui_host_app`.
- This restores proper project dependency ordering during parallel MSBuild and avoids link race failures.
- No public API/ABI changes were made.

**Validation commands run and results**
1. `cmake -S . -B build`  
Result: PASS

2. `cmake --build build --config Release --parallel`  
Result: PASS (after fix)

3. `ctest --test-dir build -C Release --output-on-failure`  
Result: PASS (`2/2` tests passed)

4. `cmake -P scripts/check-format.cmake`  
Result: PASS

Additional issue-specific check:
1. `cmake -S . -B build-gui-on -DHOTPLUGPP_BUILD_GUI_EXAMPLE=ON`  
Result: PASS with expected warning: `HOTPLUGPP_IMGUI_DIR is not set. Skipping gui_host_app.`

2. `cmake --build build-gui-on --config Release --parallel`  
Result: PASS (non-GUI targets build successfully)

**Remaining risks / assumptions**
- GUI runtime behavior was not executed locally because Dear ImGui path (`HOTPLUGPP_IMGUI_DIR`) was not provided.
- GoogleTest is not installed in this environment, so only non-GTest contract tests ran.
- Assumed existing GUI source/docs in repo satisfy planner acceptance criteria (they are present and contract tests pass).

AGENT_STATUS: {"status":"READY","findings":0}