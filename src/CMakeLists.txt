# Core library
add_library(hotplugpp STATIC
    plugin_loader.cpp
    file_watcher.cpp
)

target_include_directories(hotplugpp PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link efsw
target_link_libraries(hotplugpp PUBLIC efsw-static)

# Link platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(hotplugpp PUBLIC dl)
endif()
