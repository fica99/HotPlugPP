Implemented fix for the planner-review blocker and validated it.

Findings addressed:
1. Planner-review Finding #1 (High): incomplete ImGui source validation in `examples/gui_host_app/CMakeLists.txt` could hard-fail on partial ImGui trees.
- Fix: validation now checks all ImGui files actually used by `gui_host_app` (`imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp` included) and emits a warning + `return()` (skip target) if any are missing.

Files changed:
1. [examples/gui_host_app/CMakeLists.txt](C:/Users/ficac/Desktop/projects/my/HotPlugPP-implementer-issue-19/examples/gui_host_app/CMakeLists.txt)

Validation commands run and results:
1. `cmake -S . -B build`
- PASS

2. `cmake --build build --config Release --parallel`
- PASS

3. `ctest --test-dir build -C Release --output-on-failure`
- PASS (2/2 tests passed)

4. `cmake -P scripts/check-format.cmake`
- PASS

5. Targeted blocker regression check:
`cmake -S . -B build-gui-partial -DHOTPLUGPP_BUILD_GUI_EXAMPLE=ON -DHOTPLUGPP_IMGUI_DIR=<partial_imgui_dir>`
- PASS (configure succeeds, warning shown, `gui_host_app` skipped as intended)

Remaining unresolved findings:
- None.

Remaining risks or assumptions:
1. GUI runtime was not executed with a full real ImGui+GLFW+OpenGL dependency set in this environment; this change specifically validates graceful degradation for incomplete ImGui trees.
2. `git status` could not be run due repository safe-directory ownership policy in this sandbox, so file-change reporting is based on direct edit tracking from this session.

AGENT_STATUS: {"status":"READY","findings":0}