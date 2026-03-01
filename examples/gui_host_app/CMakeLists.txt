if(NOT WIN32)
    message(STATUS
        "HOTPLUGPP_BUILD_GUI_EXAMPLE is enabled, but gui_host_app is currently supported only on Windows. Skipping target."
    )
    return()
endif()

add_executable(gui_host_app WIN32
    main.cpp
)

target_link_libraries(gui_host_app PRIVATE
    hotplugpp
)

add_dependencies(gui_host_app sample_plugin)

add_custom_command(TARGET gui_host_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sample_plugin>
        $<TARGET_FILE_DIR:gui_host_app>/$<TARGET_FILE_NAME:sample_plugin>
    COMMENT "Copying sample_plugin next to gui_host_app"
)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set_target_properties(gui_host_app PROPERTIES
        VS_GLOBAL_BuildProjectReferences false
    )
endif()
