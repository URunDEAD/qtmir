add_library(miroil OBJECT
    compositor.cpp ${MIROIL_INCLUDE_DIRS}/miroil/compositor.h
    edid.cpp ${qtmir_include}/miroil/edid.h
    persist_display_config.cpp ${MIROIL_INCLUDE_DIRS}/miroil/persist_display_config.h
    display_configuration_policy.cpp ${qtmir_include}/miroil/display_configuration_policy.h
    display_configuration_controller_wrapper.cpp ${MIROIL_INCLUDE_DIRS}/miroil/display_configuration_controller_wrapper.h
    display_listener_wrapper.cpp ${MIROIL_INCLUDE_DIRS}/miroil/display_listener_wrapper.h
    eventdispatch.cpp ${MIROIL_INCLUDE_DIRS}/miroil/eventdispatch.h
    event_builder.cpp ${MIROIL_INCLUDE_DIRS}/miroil/event_builder.h
    input_device.cpp ${MIROIL_INCLUDE_DIRS}/miroil/input_device.h
    input_device_observer.cpp ${MIROIL_INCLUDE_DIRS}/miroil/input_device_observer.h
    mir_server_hooks.cpp ${MIROIL_INCLUDE_DIRS}/miroil/mir_server_hooks.h
    mir_prompt_session.cpp ${MIROIL_INCLUDE_DIRS}/miroil/mir_prompt_session.h
    open_gl_context.cpp ${MIROIL_INCLUDE_DIRS}/miroil/open_gl_context.h
    prompt_session_listener.cpp ${MIROIL_INCLUDE_DIRS}/miroil/prompt_session_listener.h
    prompt_session_manager.cpp ${MIROIL_INCLUDE_DIRS}/miroil/prompt_session_manager.h
    set_compositor.cpp ${MIROIL_INCLUDE_DIRS}/miroil/set_compositor.h
    surface.cpp ${MIROIL_INCLUDE_DIRS}/miroil/surface.h
    ${qtmir_include}/miroil/display_configuration_storage.h
    ${qtmir_include}/miroil/display_id.h
)

target_include_directories(miroil
    PUBLIC  "${MIROIL_INCLUDE_DIRS}" "${qtmir_include}" "${qtmir_include}/qtmir"
    PRIVATE ${MIRAL_INCLUDE_DIRS}
    ${MIRCOMMON_INCLUDE_DIRS}
    ${MIRSERVER_INCLUDE_DIRS}
    ${MIRRENDERERGLDEV_INCLUDE_DIRS}
    ${MIRSERVER_INCLUDE_DIRS}
)

target_link_libraries(miroil
    PRIVATE
    ${MIRAL_LDFLAGS}
    ${MIRSERVER_LDFLAGS}

    ${EGL_LDFLAGS}
)
