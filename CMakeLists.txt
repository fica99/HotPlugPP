cmake_minimum_required(VERSION 3.15)
project(HotPlugPP VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)
# Avoid MSBuild/ZERO_CHECK project-reference failures under parallel solution builds.
set(CMAKE_SUPPRESS_REGENERATION ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(HOTPLUGPP_USE_EFSW "Use efsw when it is available" ON)
option(HOTPLUGPP_FETCH_EFSW "Fetch efsw with FetchContent when it is not already available" ON)

set(HOTPLUGPP_HAS_EFSW OFF)
set(HOTPLUGPP_EFSW_TARGET "")

if(HOTPLUGPP_USE_EFSW)
    find_package(efsw CONFIG QUIET)

    if(NOT TARGET efsw AND NOT TARGET efsw-static AND HOTPLUGPP_FETCH_EFSW)
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
    endif()

    if(TARGET efsw OR TARGET efsw-static)
        set(HOTPLUGPP_HAS_EFSW ON)
        if(TARGET efsw)
            set(HOTPLUGPP_EFSW_TARGET efsw)
        else()
            set(HOTPLUGPP_EFSW_TARGET efsw-static)
        endif()
    else()
        message(STATUS "efsw not available; using the built-in polling watcher fallback.")
    endif()
endif()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(examples EXCLUDE_FROM_ALL)

# Convenience wrapper targets so explicit example builds work consistently
# across generators (including Visual Studio).
if(TARGET host_app)
    add_custom_target(build_host_app DEPENDS host_app)
endif()
if(TARGET sample_plugin)
    add_custom_target(build_sample_plugin DEPENDS sample_plugin)
endif()
if(TARGET math_plugin)
    add_custom_target(build_math_plugin DEPENDS math_plugin)
endif()

# Testing
option(HOTPLUGPP_BUILD_TESTS "Build tests" ON)

if(HOTPLUGPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests EXCLUDE_FROM_ALL)
    if(TARGET hotplugpp_tests)
        add_custom_target(build_hotplugpp_tests DEPENDS hotplugpp_tests)
    endif()
endif()
