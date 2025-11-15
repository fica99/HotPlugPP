# Sample Plugin (shared library)
add_library(sample_plugin SHARED
    sample_plugin/SamplePlugin.cpp
)

target_include_directories(sample_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Set output name to match platform conventions
set_target_properties(sample_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "sample_plugin"
)

if(WIN32)
    set_target_properties(sample_plugin PROPERTIES
        SUFFIX ".dll"
    )
else()
    set_target_properties(sample_plugin PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()

# Host Application
add_executable(host_app
    host_app.cpp
)

target_link_libraries(host_app PRIVATE
    hotplug
)

# Copy plugin to output directory after build
add_custom_command(TARGET sample_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:sample_plugin>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:sample_plugin>
    COMMENT "Copying sample_plugin to bin directory"
)
