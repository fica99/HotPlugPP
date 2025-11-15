# Core library
add_library(hotplugpp STATIC
    PluginLoader.cpp
)

target_include_directories(hotplugpp PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(hotplugpp PUBLIC dl)
endif()
