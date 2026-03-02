Summary of implementation status for Issue #19:

Issue #19 is already implemented in this worktree and aligns with the planner/architect handoffs. I did not need to apply additional code edits.

Files changed:
- None

Validation commands run and results:
1. `cmake -S . -B build`
- Result: PASS
- Notes: Configure succeeded; GoogleTest not found so GTest-based tests were skipped (as expected by current CMake logic).

2. `cmake --build build --config Release --parallel`
- Result: PASS
- Notes: Built `hotplugpp`, `host_app`, `sample_plugin`, `math_plugin`.

3. `ctest --test-dir build -C Release --output-on-failure`
- Result: PASS
- Notes: 2/2 tests passed:
  - `gui_example_source_contract`
  - `gui_example_docs_contract`

4. `cmake -P scripts/check-format.cmake`
- Result: PASS
- Notes: Formatting check passed for all files.

Additional issue-specific check:
5. `cmake -S . -B build-gui-missing -DHOTPLUGPP_BUILD_GUI_EXAMPLE=ON`
- Result: PASS
- Notes: Correct warning-and-skip behavior confirmed when `HOTPLUGPP_IMGUI_DIR` is unset.

Remaining risks:
- GUI target/runtime behavior with real ImGui+GLFW+OpenGL dependencies present was not fully built/executed in this run (dependency availability is environment-dependent).
- GoogleTest-based unit/integration tests did not run because GTest is not installed in this environment.

AGENT_STATUS: {"status":"PASS","findings":0}