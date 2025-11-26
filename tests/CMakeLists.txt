# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Include Google Test
include(GoogleTest)

# Platform-specific shared library prefix and suffix
if(WIN32)
    set(SHARED_LIB_PREFIX "")
    set(SHARED_LIB_SUFFIX ".dll")
elseif(APPLE)
    set(SHARED_LIB_PREFIX "lib")
    set(SHARED_LIB_SUFFIX ".dylib")
else()
    set(SHARED_LIB_PREFIX "lib")
    set(SHARED_LIB_SUFFIX ".so")
endif()

# Test plugin for integration tests
add_library(test_plugin SHARED
    test_plugin/test_plugin.cpp
)
target_include_directories(test_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
set_target_properties(test_plugin PROPERTIES
    PREFIX "${SHARED_LIB_PREFIX}"
    SUFFIX "${SHARED_LIB_SUFFIX}"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    # For multi-config generators (MSVC, Xcode), ensure DLLs go to the same location
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

# Failing test plugin (onLoad returns false)
add_library(failing_plugin SHARED
    test_plugin/failing_plugin.cpp
)
target_include_directories(failing_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
set_target_properties(failing_plugin PROPERTIES
    PREFIX "${SHARED_LIB_PREFIX}"
    SUFFIX "${SHARED_LIB_SUFFIX}"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    # For multi-config generators (MSVC, Xcode), ensure DLLs go to the same location
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

# Version tests
add_executable(version_tests
    version_tests.cpp
)
target_link_libraries(version_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
gtest_discover_tests(version_tests)

# PluginLoader tests
add_executable(plugin_loader_tests
    plugin_loader_tests.cpp
)
target_link_libraries(plugin_loader_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
target_compile_definitions(plugin_loader_tests PRIVATE
    TEST_PLUGIN_DIR="${CMAKE_BINARY_DIR}/tests"
    SHARED_LIB_PREFIX="${SHARED_LIB_PREFIX}"
    SHARED_LIB_SUFFIX="${SHARED_LIB_SUFFIX}"
)
add_dependencies(plugin_loader_tests test_plugin failing_plugin)
gtest_discover_tests(plugin_loader_tests)

# IPlugin interface tests
add_executable(iplugin_tests
    i_plugin_tests.cpp
)
target_link_libraries(iplugin_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
gtest_discover_tests(iplugin_tests)

# Integration tests
add_executable(integration_tests
    integration_tests.cpp
)
target_link_libraries(integration_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
target_compile_definitions(integration_tests PRIVATE
    TEST_PLUGIN_DIR="${CMAKE_BINARY_DIR}/tests"
    SHARED_LIB_PREFIX="${SHARED_LIB_PREFIX}"
    SHARED_LIB_SUFFIX="${SHARED_LIB_SUFFIX}"
)
add_dependencies(integration_tests test_plugin)
gtest_discover_tests(integration_tests)

# FileWatcher tests
add_executable(file_watcher_tests
    file_watcher_tests.cpp
)
target_link_libraries(file_watcher_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
target_compile_definitions(file_watcher_tests PRIVATE
    TEST_PLUGIN_DIR="${CMAKE_BINARY_DIR}/tests"
)
gtest_discover_tests(file_watcher_tests)
