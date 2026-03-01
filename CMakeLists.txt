cmake_minimum_required(VERSION 3.15)
project(HotPlugPP VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)
set(CMAKE_SUPPRESS_REGENERATION ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform-specific settings
if(WIN32)
    add_definitions(-DHOTPLUGPP_PLATFORM_WINDOWS)
elseif(UNIX)
    add_definitions(-DHOTPLUGPP_PLATFORM_UNIX)
endif()

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(HOTPLUGPP_USE_EFSW "Use efsw when it is available" ON)
option(HOTPLUGPP_FETCH_EFSW "Fetch efsw with FetchContent when it is not already available" ON)

set(HOTPLUGPP_HAS_EFSW OFF)

function(hotplugpp_can_fetch_efsw out_var)
    set(_probe_path "${CMAKE_BINARY_DIR}/CMakeFiles/hotplugpp-efsw-probe.txt")
    file(
        DOWNLOAD
        "https://raw.githubusercontent.com/SpartanJ/efsw/refs/tags/1.4.0/CMakeLists.txt"
        "${_probe_path}"
        STATUS _probe_status
        TIMEOUT 5
        LOG _probe_log
    )
    list(GET _probe_status 0 _probe_code)
    if(_probe_code EQUAL 0)
        set(${out_var} ON PARENT_SCOPE)
    else()
        set(${out_var} OFF PARENT_SCOPE)
    endif()
    file(REMOVE "${_probe_path}")
endfunction()

if(HOTPLUGPP_USE_EFSW)
    find_package(efsw CONFIG QUIET)

    if(NOT TARGET efsw AND NOT TARGET efsw-static AND HOTPLUGPP_FETCH_EFSW)
        hotplugpp_can_fetch_efsw(HOTPLUGPP_CAN_FETCH_EFSW)
    endif()

    if(NOT TARGET efsw AND NOT TARGET efsw-static AND HOTPLUGPP_FETCH_EFSW AND HOTPLUGPP_CAN_FETCH_EFSW)
        set(EFSW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(EFSW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(EFSW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            efsw
            GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
            GIT_TAG 1.4.0
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(efsw)
    elseif(NOT TARGET efsw AND NOT TARGET efsw-static AND HOTPLUGPP_FETCH_EFSW)
        message(STATUS "efsw fetch skipped because the source is unreachable; using the built-in polling watcher fallback.")
    endif()

    if(TARGET efsw OR TARGET efsw-static)
        set(HOTPLUGPP_HAS_EFSW ON)
    else()
        message(STATUS "efsw not available; using the built-in polling watcher fallback.")
    endif()
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(examples EXCLUDE_FROM_ALL)

# Testing
option(HOTPLUGPP_BUILD_TESTS "Build tests" ON)

if(HOTPLUGPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()
