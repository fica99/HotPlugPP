Summary of changes:
- Added a new CTest contract that verifies GUI configure degrades predictably when GUI deps are missing (`HOTPLUGPP_BUILD_GUI_EXAMPLE=ON` without `HOTPLUGPP_IMGUI_DIR`): configure must succeed and emit skip-warning snippets.
- Registered that new contract in the test suite.

Files changed:
- [tests/CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-tester-issue-19\tests\CMakeLists.txt)
- [tests/cmake/check_gui_example_configure_skip.cmake.in](C:\Users\ficac\Desktop\projects\my\HotPlugPP-tester-issue-19\tests\cmake\check_gui_example_configure_skip.cmake.in)

Command results (pass/fail):
- `cmake -S . -B build` -> PASS
- `cmake --build build --config Release --parallel` -> PASS
- `ctest --test-dir build -C Release --output-on-failure` -> PASS (3/3)
- `cmake -P scripts/check-format.cmake` -> PASS

Residual risks:
- GoogleTest-based suites are still skipped in this environment because GTest is not installed; only CMake contract tests executed.
- `gui_example_runtime_smoke` remains conditional on `gui_host_app` target availability and was not exercised in this run.

AGENT_STATUS: {"status":"PASS","findings":0}