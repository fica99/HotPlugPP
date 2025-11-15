# Core library
add_library(hotplug STATIC
    PluginLoader.cpp
)

target_include_directories(hotplug PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(hotplug PUBLIC dl)
endif()
