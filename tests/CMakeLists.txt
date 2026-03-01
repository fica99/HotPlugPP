option(HOTPLUGPP_FETCH_GOOGLETEST "Fetch GoogleTest with FetchContent" OFF)

find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND AND HOTPLUGPP_FETCH_GOOGLETEST)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

if(NOT GTest_FOUND AND NOT TARGET GTest::gtest_main)
    message(STATUS "GoogleTest not available; using the bundled test shim.")

    add_library(hotplugpp_test_main STATIC
        support/test_main.cpp
    )
    set_target_properties(hotplugpp_test_main PROPERTIES
        EXCLUDE_FROM_ALL ON
    )
    target_include_directories(hotplugpp_test_main PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/support
    )

    add_library(GTest::gtest_main ALIAS hotplugpp_test_main)
    set(HOTPLUGPP_USE_BUNDLED_GTEST_SHIM ON)
endif()

if(NOT HOTPLUGPP_USE_BUNDLED_GTEST_SHIM)
    include(GoogleTest)
endif()

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
set_target_properties(test_plugin PROPERTIES
    EXCLUDE_FROM_ALL ON
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
set_target_properties(failing_plugin PROPERTIES
    EXCLUDE_FROM_ALL ON
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

# Consolidated test runner keeps MSBuild project-reference resolution stable in
# parallel Visual Studio builds.
add_executable(hotplugpp_tests
    version_tests.cpp
    plugin_loader_tests.cpp
    i_plugin_tests.cpp
    integration_tests.cpp
)
set_target_properties(hotplugpp_tests PROPERTIES
    EXCLUDE_FROM_ALL ON
)
target_link_libraries(hotplugpp_tests PRIVATE
    GTest::gtest_main
    hotplugpp
)
target_compile_definitions(hotplugpp_tests PRIVATE
    TEST_PLUGIN_DIR="${CMAKE_BINARY_DIR}/tests"
    SHARED_LIB_PREFIX="${SHARED_LIB_PREFIX}"
    SHARED_LIB_SUFFIX="${SHARED_LIB_SUFFIX}"
)
add_dependencies(hotplugpp_tests test_plugin failing_plugin)

add_test(
    NAME hotplugpp_tests
    COMMAND ${CMAKE_COMMAND}
        -DBINARY_DIR=${CMAKE_BINARY_DIR}
        -DTEST_TARGET=hotplugpp_tests
        -DTEST_EXECUTABLE=$<TARGET_FILE:hotplugpp_tests>
        -DTEST_CONFIG=$<CONFIG>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/support/run_built_test.cmake
)
