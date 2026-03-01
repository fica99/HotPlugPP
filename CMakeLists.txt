cmake_minimum_required(VERSION 3.15)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_SUPPRESS_REGENERATION ON)
    set(CMAKE_VS_GLOBALS "BuildInParallel=false")
endif()

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

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

if(WIN32)
    set(HOTPLUGPP_BUILD_GUI_EXAMPLE_DEFAULT ON)
else()
    set(HOTPLUGPP_BUILD_GUI_EXAMPLE_DEFAULT OFF)
endif()

option(HOTPLUGPP_BUILD_GUI_EXAMPLE "Build the optional GUI host example"
       ${HOTPLUGPP_BUILD_GUI_EXAMPLE_DEFAULT})

# Add subdirectories
add_subdirectory(src)
add_subdirectory(examples)

# Testing
option(HOTPLUGPP_BUILD_TESTS "Build tests" ON)

if(HOTPLUGPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
