add_library(miroil OBJECT
    compositor.cpp ${miroil_include}/miroil/compositor.h    
    edid.cpp ${qtmir_include}/qtmir/miroil/edid.h
    mirbuffer.cpp ${miroil_include}/miroil/mirbuffer.h
    persist_display_config.cpp ${miroil_include}/miroil/persist_display_config.h
    display_configuration_policy.cpp ${qtmir_include}/qtmir/miroil/display_configuration_policy.h
    display_configuration_controller_wrapper.cpp ${miroil_include}/miroil/display_configuration_controller_wrapper.h
    display_listener_wrapper.cpp ${miroil_include}/miroil/display_listener_wrapper.h
    eventdispatch.cpp ${miroil_include}/miroil/eventdispatch.h
    event_builder.cpp ${miroil_include}/miroil/event_builder.h
    input_device.cpp ${miroil_include}/miroil/input_device.h
    input_device_observer.cpp ${miroil_include}/miroil/input_device_observer.h
    mir_server_hooks.cpp ${miroil_include}/miroil/mir_server_hooks.h
    mir_prompt_session.cpp ${miroil_include}/miroil/mir_prompt_session.h
    open_gl_context.cpp ${miroil_include}/miroil/open_gl_context.h    
    prompt_session_listener.cpp ${miroil_include}/miroil/prompt_session_listener.h
    prompt_session_manager.cpp ${miroil_include}/miroil/prompt_session_manager.h    
    set_compositor.cpp ${miroil_include}/miroil/set_compositor.h
    surface.cpp ${miroil_include}/miroil/surface.h
    ${qtmir_include}/qtmir/miroil/display_configuration_storage.h
    ${qtmir_include}/qtmir/miroil/display_id.h
)

target_include_directories(miroil
    PUBLIC  "${miroil_include}" "${qtmir_include}" "${qtmir_include}/qtmir"
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
