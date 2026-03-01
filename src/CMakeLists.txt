# Core library
add_library(hotplugpp STATIC
    plugin_watcher.cpp
    plugin_loader.cpp
)

find_package(Threads REQUIRED)

target_include_directories(hotplugpp PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(hotplugpp PUBLIC
    Threads::Threads
)

if(HOTPLUGPP_HAS_EFSW)
    if(HOTPLUGPP_EFSW_TARGET)
        target_link_libraries(hotplugpp PRIVATE ${HOTPLUGPP_EFSW_TARGET})
    endif()

    target_compile_definitions(hotplugpp PRIVATE HOTPLUGPP_HAS_EFSW=1)
endif()

# Link platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(hotplugpp PUBLIC dl)
endif()
