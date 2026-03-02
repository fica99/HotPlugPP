# Changelog

All notable changes to this project are documented in this file.

## [Unreleased]

### Added
- Issue #19: Added optional `gui_host_app` example (`examples/gui_host_app/`) using Dear ImGui + GLFW + OpenGL.
- Added `HOTPLUGPP_BUILD_GUI_EXAMPLE` CMake option (default `OFF`) to gate GUI example builds.
- Added GUI example integration checks:
  - `gui_example_source_contract`
  - `gui_example_docs_contract`
  - `gui_example_configure_skip_contract`
  - `gui_example_artifacts` (when GUI target exists)
- Added optional GUI runtime smoke test `gui_example_runtime_smoke` (runs `gui_host_app --smoke-test` when GUI target exists).

### Changed
- `examples/CMakeLists.txt` now conditionally adds `examples/gui_host_app/` when GUI option is enabled.
- GUI example CMake flow now degrades predictably: missing `HOTPLUGPP_IMGUI_DIR`, missing ImGui backend files, missing `glfw3`, or missing `OpenGL::GL` emit warnings and skip `gui_host_app` without failing configure.
- README now includes GUI sample build/run instructions and dependency requirements.

