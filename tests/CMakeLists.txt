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

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cmake")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_gui_example_source.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/check_gui_example_source.cmake"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_gui_example_docs.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/check_gui_example_docs.cmake"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_gui_example_artifacts.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/check_gui_example_artifacts.cmake"
    @ONLY
)

add_test(
    NAME gui_example_source_contract
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/check_gui_example_source.cmake"
)

add_test(
    NAME gui_example_docs_contract
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/check_gui_example_docs.cmake"
)

if(TARGET gui_host_app AND TARGET sample_plugin)
    add_test(
        NAME gui_example_artifacts
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/check_gui_example_artifacts.cmake"
    )
endif()

option(HOTPLUGPP_FETCH_GTEST "Download GoogleTest during configure when it is not installed" OFF)

find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND AND HOTPLUGPP_FETCH_GTEST)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

if(TARGET GTest::gtest_main)
    include(GoogleTest)

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
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
    )

    add_executable(version_tests
        version_tests.cpp
    )
    target_link_libraries(version_tests PRIVATE
        GTest::gtest_main
        hotplugpp
    )
    gtest_discover_tests(version_tests)

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

    add_executable(iplugin_tests
        i_plugin_tests.cpp
    )
    target_link_libraries(iplugin_tests PRIVATE
        GTest::gtest_main
        hotplugpp
    )
    gtest_discover_tests(iplugin_tests)

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
else()
    message(STATUS
        "GoogleTest was not found. Skipping GoogleTest-based tests. Install GTest or set HOTPLUGPP_FETCH_GTEST=ON to enable them."
    )
endif()
