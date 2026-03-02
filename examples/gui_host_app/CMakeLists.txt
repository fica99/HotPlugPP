set(HOTPLUGPP_IMGUI_DIR "" CACHE PATH
    "Path to a Dear ImGui source checkout (must contain imgui.h and backends/)"
)

if(NOT HOTPLUGPP_IMGUI_DIR)
    message(WARNING
        "HOTPLUGPP_BUILD_GUI_EXAMPLE is enabled, but HOTPLUGPP_IMGUI_DIR is not set. Skipping gui_host_app."
    )
    return()
endif()

set(HOTPLUGPP_GUI_IMGUI_REQUIRED_FILES
    imgui.h
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
)

set(HOTPLUGPP_GUI_IMGUI_MISSING_FILES)
foreach(_imgui_file IN LISTS HOTPLUGPP_GUI_IMGUI_REQUIRED_FILES)
    if(NOT EXISTS "${HOTPLUGPP_IMGUI_DIR}/${_imgui_file}")
        list(APPEND HOTPLUGPP_GUI_IMGUI_MISSING_FILES "${_imgui_file}")
    endif()
endforeach()

if(HOTPLUGPP_GUI_IMGUI_MISSING_FILES)
    string(JOIN ", " _imgui_missing_text ${HOTPLUGPP_GUI_IMGUI_MISSING_FILES})
    message(WARNING
        "HOTPLUGPP_BUILD_GUI_EXAMPLE is enabled, but HOTPLUGPP_IMGUI_DIR is missing required Dear ImGui file(s): ${_imgui_missing_text}. Skipping gui_host_app."
    )
    return()
endif()

find_package(glfw3 QUIET)
if(TARGET glfw)
    set(HOTPLUGPP_GUI_GLFW_TARGET glfw)
elseif(TARGET glfw3)
    set(HOTPLUGPP_GUI_GLFW_TARGET glfw3)
elseif(TARGET glfw3::glfw)
    set(HOTPLUGPP_GUI_GLFW_TARGET glfw3::glfw)
else()
    message(WARNING
        "HOTPLUGPP_BUILD_GUI_EXAMPLE is enabled, but no glfw target was found via find_package(glfw3). Skipping gui_host_app."
    )
    return()
endif()

find_package(OpenGL QUIET)
if(NOT TARGET OpenGL::GL)
    message(WARNING
        "HOTPLUGPP_BUILD_GUI_EXAMPLE is enabled, but OpenGL::GL is unavailable. Skipping gui_host_app."
    )
    return()
endif()

add_executable(gui_host_app
    main.cpp
    ${HOTPLUGPP_IMGUI_DIR}/imgui.cpp
    ${HOTPLUGPP_IMGUI_DIR}/imgui_draw.cpp
    ${HOTPLUGPP_IMGUI_DIR}/imgui_tables.cpp
    ${HOTPLUGPP_IMGUI_DIR}/imgui_widgets.cpp
    ${HOTPLUGPP_IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${HOTPLUGPP_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(gui_host_app PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${HOTPLUGPP_IMGUI_DIR}
    ${HOTPLUGPP_IMGUI_DIR}/backends
)

target_link_libraries(gui_host_app PRIVATE
    hotplugpp
    ${HOTPLUGPP_GUI_GLFW_TARGET}
    OpenGL::GL
)

add_dependencies(gui_host_app sample_plugin)

add_custom_command(TARGET gui_host_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sample_plugin>
        $<TARGET_FILE_DIR:gui_host_app>/$<TARGET_FILE_NAME:sample_plugin>
    COMMENT "Copying sample_plugin next to gui_host_app"
)

