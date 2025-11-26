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

# Test plugin for integration tests
add_library(test_plugin SHARED
    test_plugin/TestPlugin.cpp
)
target_include_directories(test_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
set_target_properties(test_plugin PROPERTIES
    PREFIX "lib"
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# Failing test plugin (onLoad returns false)
add_library(failing_plugin SHARED
    test_plugin/FailingPlugin.cpp
)
target_include_directories(failing_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
set_target_properties(failing_plugin PROPERTIES
    PREFIX "lib"
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

# Version tests
add_executable(version_tests
    VersionTests.cpp
)
target_link_libraries(version_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
gtest_discover_tests(version_tests)

# PluginLoader tests
add_executable(plugin_loader_tests
    PluginLoaderTests.cpp
)
target_link_libraries(plugin_loader_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
target_compile_definitions(plugin_loader_tests PRIVATE
    TEST_PLUGIN_DIR="${CMAKE_BINARY_DIR}/tests"
)
add_dependencies(plugin_loader_tests test_plugin failing_plugin)
gtest_discover_tests(plugin_loader_tests)

# IPlugin interface tests
add_executable(iplugin_tests
    IPluginTests.cpp
)
target_link_libraries(iplugin_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
gtest_discover_tests(iplugin_tests)

# Integration tests
add_executable(integration_tests
    IntegrationTests.cpp
)
target_link_libraries(integration_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
target_compile_definitions(integration_tests PRIVATE
    TEST_PLUGIN_DIR="${CMAKE_BINARY_DIR}/tests"
)
add_dependencies(integration_tests test_plugin)
gtest_discover_tests(integration_tests)
