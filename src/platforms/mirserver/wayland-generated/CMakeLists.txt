function(add_wayland_protocol PREFIX PROTOCOL)
    set(PROTOCOL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../wayland-protocols/${PROTOCOL}.xml")

    set(GENERATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${PROTOCOL}_wrapper")

    add_custom_command(
        OUTPUT ${GENERATE_FILE}.cpp
        OUTPUT ${GENERATE_FILE}.h
        DEPENDS ${PROTOCOL_FILE}
        COMMAND "sh" "-c" "/usr/bin/mir_wayland_generator ${PREFIX} ${PROTOCOL_FILE} header >${GENERATE_FILE}.h"
        COMMAND "sh" "-c" "/usr/bin/mir_wayland_generator ${PREFIX} ${PROTOCOL_FILE} source >${GENERATE_FILE}.cpp"
    )

    set(GENERATED_FILES ${GENERATED_FILES} "${GENERATE_FILE}.h" "${GENERATE_FILE}.cpp" PARENT_SCOPE)
endfunction()

add_wayland_protocol("qt_" "qt-windowmanager")

add_custom_target(refresh-wayland-wrapper
    DEPENDS ${GENERATED_FILES}
    SOURCES ${GENERATED_FILES}
)
