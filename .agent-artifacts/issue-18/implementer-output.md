Implemented Issue #18 in this worktree by wiring resilient `efsw` dependency selection in CMake and adding a bundled `efsw`-compatible shim fallback for offline/unavailable dependency cases, while preserving `PluginLoader` ABI.

**Files changed**
- [CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-implementer-issue-18\CMakeLists.txt)
- [src/CMakeLists.txt](C:\Users\ficac\Desktop\projects\my\HotPlugPP-implementer-issue-18\src\CMakeLists.txt)
- [src/compat/efsw/efsw.hpp](C:\Users\ficac\Desktop\projects\my\HotPlugPP-implementer-issue-18\src\compat\efsw\efsw.hpp)

**Validation commands run and results**
1. `cmake -S . -B build`  
Result: PASS
2. `cmake --build build --config Release --parallel`  
Result: PASS
3. `ctest --test-dir build -C Release --output-on-failure`  
Result: PASS
4. `cmake -P scripts/check-format.cmake`  
Result: PASS

Note: an intermediate `ctest` run failed once due a file-lock race when build and test were launched in parallel; rerunning `ctest` sequentially after build passed cleanly.

**Remaining risks**
- Real upstream `efsw` behavior is not validated in this offline environment; current run exercised bundled shim fallback path.
- Cross-platform runtime watcher semantics (Windows/Linux/macOS event differences) still need CI confirmation on all target OSes.
- Security/static analysis step (cppcheck/clang-tidy) was not run in this implementer pass.

AGENT_STATUS: {"status":"PASS","findings":0}