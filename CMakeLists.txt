cmake_minimum_required(VERSION 3.15)
project(HotPlugPP VERSION 1.0.0 LANGUAGES CXX)

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

# Fetch efsw (Entropia File System Watcher)
include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_TEST_APP OFF CACHE BOOL "" FORCE)
set(EFSW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    efsw
    GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
    GIT_TAG        1.4.1
)
FetchContent_MakeAvailable(efsw)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(examples)

# Testing
option(HOTPLUGPP_BUILD_TESTS "Build tests" ON)

if(HOTPLUGPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
