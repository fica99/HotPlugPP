option(HOTPLUGPP_FETCH_GOOGLETEST "Fetch GoogleTest with FetchContent" OFF)

set(HOTPLUGPP_TEST_SUPPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/support")

find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND AND HOTPLUGPP_FETCH_GOOGLETEST)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
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

set(HOTPLUGPP_TEST_FRAMEWORK_TARGET "")
set(HOTPLUGPP_TEST_MAIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/support/test_main.cpp")
set(HOTPLUGPP_TEST_INCLUDE_DIRS "")

if(TARGET GTest::gtest)
    set(HOTPLUGPP_TEST_FRAMEWORK_TARGET GTest::gtest)
elseif(TARGET GTest::gtest_main)
    set(HOTPLUGPP_TEST_FRAMEWORK_TARGET GTest::gtest_main)
    set(HOTPLUGPP_TEST_MAIN_SOURCE "")
else()
    message(STATUS "GoogleTest not available; using the bundled test shim.")
    list(APPEND HOTPLUGPP_TEST_INCLUDE_DIRS ${HOTPLUGPP_TEST_SUPPORT_DIR})
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

set(HOTPLUGPP_TEST_SOURCES
    version_tests.cpp
    plugin_loader_tests.cpp
    i_plugin_tests.cpp
    integration_tests.cpp
)

if(HOTPLUGPP_TEST_MAIN_SOURCE)
    list(APPEND HOTPLUGPP_TEST_SOURCES ${HOTPLUGPP_TEST_MAIN_SOURCE})
endif()

add_executable(hotplugpp_tests
    ${HOTPLUGPP_TEST_SOURCES}
)
if(HOTPLUGPP_TEST_INCLUDE_DIRS)
    target_include_directories(hotplugpp_tests PRIVATE
        ${HOTPLUGPP_TEST_INCLUDE_DIRS}
    )
endif()
target_link_libraries(hotplugpp_tests PRIVATE
    hotplugpp
)
if(HOTPLUGPP_TEST_FRAMEWORK_TARGET)
    target_link_libraries(hotplugpp_tests PRIVATE ${HOTPLUGPP_TEST_FRAMEWORK_TARGET})
endif()
target_compile_definitions(hotplugpp_tests PRIVATE
    TEST_PLUGIN_DIR="${CMAKE_BINARY_DIR}/tests"
    SHARED_LIB_PREFIX="${SHARED_LIB_PREFIX}"
    SHARED_LIB_SUFFIX="${SHARED_LIB_SUFFIX}"
)
add_dependencies(hotplugpp_tests test_plugin failing_plugin)

add_test(
    NAME hotplugpp_tests
    COMMAND
        ${CMAKE_COMMAND}
        -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DTEST_TARGET=hotplugpp_tests
        -DTEST_BINARY=$<TARGET_FILE:hotplugpp_tests>
        -DTEST_WORKING_DIR=$<TARGET_FILE_DIR:hotplugpp_tests>
        -DTEST_CONFIG=$<CONFIG>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/support/run_built_test.cmake
)
