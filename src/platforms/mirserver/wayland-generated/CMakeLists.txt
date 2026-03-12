function(add_wayland_protocol PREFIX PROTOCOL)
    set(PROTOCOL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../wayland-protocols/${PROTOCOL}.xml")

    set(GENERATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${PROTOCOL}_wrapper")

    if(WITH_MIR2)
        find_program(MIR_WAYLAND_GENERATOR mir_wayland_generator)
    else()
        find_program(MIR_WAYLAND_GENERATOR mir1_wayland_generator)
    endif()

    add_custom_command(
        OUTPUT ${GENERATE_FILE}.cpp
        OUTPUT ${GENERATE_FILE}.h
        DEPENDS ${MIR_WAYLAND_GENERATOR}
        DEPENDS ${PROTOCOL_FILE}
        COMMAND "sh" "-c" "${MIR_WAYLAND_GENERATOR} ${PREFIX} ${PROTOCOL_FILE} header > ${GENERATE_FILE}.h"
        COMMAND "sh" "-c" "${MIR_WAYLAND_GENERATOR} ${PREFIX} ${PROTOCOL_FILE} source > ${GENERATE_FILE}.cpp"
        VERBATIM
    )

    set(GENERATED_FILES ${GENERATED_FILES} "${GENERATE_FILE}.h" "${GENERATE_FILE}.cpp" PARENT_SCOPE)
endfunction()

add_wayland_protocol("qt_" "qt-windowmanager")

add_custom_target(refresh-wayland-wrapper
    DEPENDS ${GENERATED_FILES}
    SOURCES ${GENERATED_FILES}
)
