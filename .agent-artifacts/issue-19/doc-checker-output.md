Issue #19 documentation audit is complete.

Documentation gaps found:
- `README.md` did not document the new `gui_host_app --smoke-test` runtime mode added with this issue.
- `CONTRIBUTING.md` “Running Tests” section was outdated relative to current required CMake/CTest/format validation flow and did not mention optional GUI smoke validation.
- `CHANGELOG.md` did not exist, so Issue #19 changes were not tracked.

Files updated:
- [README.md](/C:/Users/ficac/Desktop/projects/my/HotPlugPP-doc-checker-issue-19/README.md)
- [CONTRIBUTING.md](/C:/Users/ficac/Desktop/projects/my/HotPlugPP-doc-checker-issue-19/CONTRIBUTING.md)
- [CHANGELOG.md](/C:/Users/ficac/Desktop/projects/my/HotPlugPP-doc-checker-issue-19/CHANGELOG.md)

What was verified against code changes:
- New feature documented: optional GUI host example (`examples/gui_host_app/`) with Load/Unload/Check-Reload flow.
- New build behavior documented: `HOTPLUGPP_BUILD_GUI_EXAMPLE` and `HOTPLUGPP_IMGUI_DIR`, including dependency-missing skip behavior.
- New test behavior documented: optional `gui_example_runtime_smoke` via `gui_host_app --smoke-test`.
- No public API changes in `include/` were introduced by this issue.

Validation commands run and results:
- `cmake -S . -B build` -> PASS
- `cmake --build build --config Release --parallel` -> PASS
- `ctest --test-dir build -C Release --output-on-failure` -> PASS (3/3)
- `cmake -P scripts/check-format.cmake` -> PASS

Remaining risks or assumptions:
- `gui_example_runtime_smoke` remains conditional on GUI deps (`HOTPLUGPP_IMGUI_DIR`, `glfw3`, `OpenGL`) and was not executed in this environment.
- SecurityChecker previously noted possible null C-string rendering risk in GUI metadata display; this is unchanged by doc-only updates.

AGENT_STATUS: {"status":"PASS","findings":0}