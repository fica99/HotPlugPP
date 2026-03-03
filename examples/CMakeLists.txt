# Platform-specific plugin suffix/prefix (shared between all plugin targets below).
if(WIN32)
    set(HOTPLUGPP_EXAMPLE_PLUGIN_PREFIX "")
    set(HOTPLUGPP_EXAMPLE_PLUGIN_SUFFIX ".dll")
elseif(APPLE)
    set(HOTPLUGPP_EXAMPLE_PLUGIN_PREFIX "lib")
    set(HOTPLUGPP_EXAMPLE_PLUGIN_SUFFIX ".dylib")
else()
    set(HOTPLUGPP_EXAMPLE_PLUGIN_PREFIX "lib")
    set(HOTPLUGPP_EXAMPLE_PLUGIN_SUFFIX ".so")
endif()

# Sample Plugin (shared library)
add_library(sample_plugin SHARED
    sample_plugin/sample_plugin.cpp
)
target_include_directories(sample_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
set_target_properties(sample_plugin PROPERTIES
    EXCLUDE_FROM_ALL ON
    PREFIX "${HOTPLUGPP_EXAMPLE_PLUGIN_PREFIX}"
    OUTPUT_NAME "sample_plugin"
    SUFFIX "${HOTPLUGPP_EXAMPLE_PLUGIN_SUFFIX}"
)

# Math Plugin (shared library)
add_library(math_plugin SHARED
    math_plugin/math_plugin.cpp
)
target_include_directories(math_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
set_target_properties(math_plugin PROPERTIES
    EXCLUDE_FROM_ALL ON
    PREFIX "${HOTPLUGPP_EXAMPLE_PLUGIN_PREFIX}"
    OUTPUT_NAME "math_plugin"
    SUFFIX "${HOTPLUGPP_EXAMPLE_PLUGIN_SUFFIX}"
)

# Host Application
add_executable(host_app
    host_app.cpp
)
set_target_properties(host_app PROPERTIES
    EXCLUDE_FROM_ALL ON
)

target_link_libraries(host_app PRIVATE
    hotplugpp
)

# Copy plugins to output directory after build
add_custom_command(TARGET sample_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:sample_plugin>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:sample_plugin>
    COMMENT "Copying sample_plugin to bin directory"
)

add_custom_command(TARGET math_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:math_plugin>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:math_plugin>
    COMMENT "Copying math_plugin to bin directory"
)
