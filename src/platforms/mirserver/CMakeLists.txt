# Qt5PlatformSupport is not available as a cmake module.
# And also not anymore as pkgconfig module since Qt 5.6.
#pkg_check_modules(QT5PLATFORM_SUPPORT Qt5PlatformSupport REQUIRED)
include(FindQt5PlatformSupport)
include(UseLttngGenTp)

set(INCLUDE_NAME "qtmir-qt${QT_VERSION_MAJOR}-mir${MIR_VERSION_MAJOR}")
set(QTMIRSERVER_SONAME "qt${QT_VERSION_MAJOR}mir${MIR_VERSION_MAJOR}server")
set(QPA_MIRSERVER_NAME "mir${MIR_VERSION_MAJOR}server")
set(QPA_MIRSERVER_SONAME "qpa-${QPA_MIRSERVER_NAME}")

# Used by src/modules and demos/ directories.
set(QPA_MIRSERVER_NAME "${QPA_MIRSERVER_NAME}" PARENT_SCOPE)

add_definitions(-DQPA_MIRSERVER_NAME=\"${QPA_MIRSERVER_NAME}\")

if(WITH_MIR2)
    # TODO: read MirOil and MirAL's SOVERSION, and take that into our SOVERSION.
    set(QTMIRSERVER_ABI 1)
else()
    # We don't expect to ever bump ABI again.
    set(QTMIRSERVER_ABI 1)
endif()

set(qtmir_include ${CMAKE_SOURCE_DIR}/include)

if(NOT WITH_MIR2)
    set(MIROIL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/miroil/include)
    add_subdirectory(miroil)
endif()

add_subdirectory(wayland-generated)

# Hacks for the QPA privates monster.
pkg_check_modules(FONTCONFIG fontconfig REQUIRED)

if (QGL_DEBUG)
    message(STATUS "Qt's OpenGL debug logging enabled.")
    add_definitions(-DQGL_DEBUG)
endif()

if (WITH_CONTENTHUB)
    add_definitions(-DWITH_CONTENTHUB)
endif()

find_package(Qt5FontDatabaseSupport REQUIRED)
find_package(Qt5ThemeSupport REQUIRED)
find_package(Qt5EventDispatcherSupport REQUIRED)
find_package(Qt5EglSupport REQUIRED)
find_package(Qt5ServiceSupport REQUIRED)
set(QT5PLATFORMSUPPORT_LIBS
    Qt5FontDatabaseSupport::Qt5FontDatabaseSupport
    Qt5ThemeSupport::Qt5ThemeSupport
    Qt5EventDispatcherSupport::Qt5EventDispatcherSupport
    Qt5EglSupport::Qt5EglSupport
    Qt5ServiceSupport::Qt5ServiceSupport
)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_BINARY_DIR} # for tracepoints.h
)

include_directories(
    SYSTEM
    ${MIRAL_INCLUDE_DIRS}
    ${MIRCOMMON_INCLUDE_DIRS}
    ${MIRSERVER_INCLUDE_DIRS}
    ${MIRRENDERERGLDEV_INCLUDE_DIRS}
    ${MIRSERVER_INCLUDE_DIRS}
    ${MIROIL_INCLUDE_DIRS}
    ${MIRWAYLAND_INCLUDE_DIRS}

    ${URL_DISPATCHER_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${LTTNG_INCLUDE_DIRS}

    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}


    ${Qt5Quick_INCLUDE_DIRS}
    ${Qt5Quick_PRIVATE_INCLUDE_DIRS}
    ${Qt5Qml_INCLUDE_DIRS}

if(WITH_CONTENTHUB)
    ${CONTENT_HUB_INCLUDE_DIRS}
endif()

    ${VALGRIND_INCLUDE_DIRS}
)

# We have to remove -pedantic for tracepoints.c
string (REPLACE " -pedantic " " " TRACEPOINTS_CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
# BYTE_ORDER needed to compile tracepoints in C99 mode.
set(TRACEPOINTS_CMAKE_C_FLAGS "${TRACEPOINTS_CMAKE_C_FLAGS} -DBYTE_ORDER=__BYTE_ORDER -fPIC ")
set_source_files_properties(tracepoints.c PROPERTIES COMPILE_FLAGS "${TRACEPOINTS_CMAKE_C_FLAGS}")

# Generate tracepoints.c and .h from tracepoints.tp
add_lttng_gen_tp(NAME tracepoints)
add_custom_target(MirServer_LTTNG
    DEPENDS tracepoints.h tracepoints.c
)

if(WITH_CONTENTHUB)
    set(CLIPBOARD_SRC clipboard.cpp)
endif()

if(NOT WITH_MIR2)
    set(PROMPTSESSION_NOMIRSERVER_SRC promptsessionlistener.cpp)
    set(PROMPTSESSION_MIRSERVER_SRC promptsessionmanager.cpp promptsessionmanager.h promptsession.h)
endif()

# These files will compile without mirserver-dev
add_library(qtmirserver-nomirserver OBJECT
    ${CMAKE_SOURCE_DIR}/src/common/timestamp.cpp
    ${CLIPBOARD_SRC}
    logging.cpp
    shelluuid.cpp
    mirsingleton.cpp
    wrappedsessionauthorizer.cpp
    ${PROMPTSESSION_NOMIRSERVER_SRC}
    displayconfigurationstorage.cpp
    nativeinterface.cpp
    services.cpp
    urlbuilderdispatcher.cpp
    windowcontroller.cpp
    workspacecontroller.cpp
    tracepoints.c
    surfaceobserver.cpp
    initialsurfacesizes.cpp
    orientationsensor.cpp
    screenadaptor.cpp
    screenadaptormodel.cpp
    qtmir.cpp
)
add_dependencies(qtmirserver-nomirserver MirServer_LTTNG)

include_directories(SYSTEM ${MIRSERVER_INCLUDE_DIRS})

# These files will only compile with mirserver-dev
add_library(qtmirserver-mirserver OBJECT
    screenscontroller.cpp
    qtcompositor.cpp
    displayconfigurationpolicy.cpp
    wrappedwindowmanagementpolicy.cpp
    ${PROMPTSESSION_MIRSERVER_SRC}
)
add_dependencies(qtmirserver-mirserver MirServer_LTTNG)

# These files get entangled by automoc so they need to go together. And some depend on mirserver-dev
set(QMIRSERVER_PLUGIN_SRC
    ${CMAKE_SOURCE_DIR}/src/common/debughelpers.cpp
    cursor.cpp
    eventbuilder.cpp
    qtwindowmanager.cpp
    qteventfeeder.cpp
    qmirserver.cpp
    qmirserver_p.cpp
    platformscreen.cpp
    screenplatformwindow.cpp
    screensmodel.cpp
    miropenglcontext.cpp
    offscreensurface.cpp
    inputdeviceobserver.cpp
    mirserverapplication.cpp
    windowmodelnotifier.cpp

# We need to run moc on these headers
    ${CMAKE_SOURCE_DIR}/include/lomiri/shell/application/MirMousePointerInterface.h
    ${CMAKE_SOURCE_DIR}/include/lomiri/shell/application/Mir.h

    ${CMAKE_SOURCE_DIR}/include/qtmir/mirserverapplication.h
    ${CMAKE_SOURCE_DIR}/include/qtmir/displayconfigurationpolicy.h
    ${CMAKE_SOURCE_DIR}/include/qtmir/sessionauthorizer.h
    ${CMAKE_SOURCE_DIR}/include/qtmir/windowmanagementpolicy.h
    ${CMAKE_SOURCE_DIR}/include/qtmir/displayconfigurationstorage.h
    ${CMAKE_SOURCE_DIR}/include/qtmir/screen.h
    ${CMAKE_SOURCE_DIR}/include/qtmir/screens.h
    ${CMAKE_SOURCE_DIR}/include/qtmir/types.h
    ${CMAKE_SOURCE_DIR}/include/qtmir/qtmir.h

    ${CMAKE_SOURCE_DIR}/src/common/windowcontrollerinterface.h
    ${CMAKE_SOURCE_DIR}/src/common/workspacecontrollerinterface.h
    ${CMAKE_SOURCE_DIR}/src/common/windowmodelnotifier.h
    ${CMAKE_SOURCE_DIR}/src/common/appnotifier.h
)

if(NOT WITH_MIR2)
    set(MIROIL_OBJ $<TARGET_OBJECTS:miroil>)
endif()

add_library(qtmirserver SHARED
    ${QMIRSERVER_PLUGIN_SRC}
    $<TARGET_OBJECTS:qtmirserver-nomirserver>
    $<TARGET_OBJECTS:qtmirserver-mirserver>
    ${MIROIL_OBJ}
    $<TARGET_OBJECTS:wayland-protocols>
)

target_link_libraries(qtmirserver
    PUBLIC
        ${MIRAL_LDFLAGS}
        ${MIROIL_LDFLAGS}
        ${MIRSERVER_LDFLAGS}
        Qt5::Core
        Qt5::Gui

    PRIVATE
        Threads::Threads
        ${URL_DISPATCHER_LDFLAGS}
        ${EGL_LDFLAGS}
        ${LTTNG_LIBRARIES}
        ${GIO_LDFLAGS}
        ${FONTCONFIG_LDFLAGS}
        ${XKBCOMMON_LIBRARIES}

        ${MIRCOMMON_LDFLAGS}
        ${MIRWAYLAND_LDFLAGS}
        ${WAYLAND_CLIENT_LIBRARIES}
        ${WAYLAND_SERVER_LDFLAGS}

        ${CONTENT_HUB_LIBRARIES}

        Qt5::DBus
        Qt5::Quick
        Qt5::Sensors
        ${QT5PLATFORMSUPPORT_LIBS}
)

set_target_properties(qtmirserver
  PROPERTIES
  OUTPUT_NAME ${QTMIRSERVER_SONAME}
  SOVERSION ${QTMIRSERVER_ABI}
)

set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${INCLUDE_NAME}")
if(WITH_MIR2)
    set(QTMIRSERVER_PCREQUIRES "miral >= 3.0, miroil")
else()
    set(QTMIRSERVER_PCREQUIRES "${MIRAL_MODULE_NAME} >= 1.1")
    set(QTMIRSERVER_PCCONFLICTS "miral >= 3.0")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/qtXmirYserver.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${QTMIRSERVER_SONAME}.pc
    @ONLY
)

install(
  TARGETS qtmirserver
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/qtmir
  DESTINATION ${INCLUDEDIR}
)

if(NOT WITH_MIR2)
    install(
      DIRECTORY ${CMAKE_SOURCE_DIR}/include/miroil
      DESTINATION ${INCLUDEDIR}
    )
endif()

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${QTMIRSERVER_SONAME}.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Lomiri shell Application API: header-only interfaces that qtmir implements.
# Kept as its own pkg-config module (versioned independently of qtmirserver's
# SOVERSION) but shipped in the same libqt5mir1server-dev package, since a
# shell consuming it already needs to build against a Mir server implementing
# the interface.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lomiri-shell-application.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/lomiri-shell-application.pc
    @ONLY
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/lomiri
  DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/lomiri-shell-api
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/lomiri-shell-application.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

configure_file(mirserver.json.in mirserver.json)

add_library(qpa-mirserver SHARED
    mirserverintegration.cpp
    plugin.cpp
)

set_target_properties(qpa-mirserver
  PROPERTIES
  OUTPUT_NAME ${QPA_MIRSERVER_SONAME})

target_link_libraries(qpa-mirserver
PRIVATE
    qtmirserver
    ${QT5PLATFORMSUPPORT_LIBS}

    ${MIRCOMMON_LDFLAGS}
    ${MIRSERVER_LDFLAGS}
    ${MIRWAYLAND_LDFLAGS}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_SERVER_LDFLAGS}

    ${URL_DISPATCHER_LDFLAGS}
    ${EGL_LDFLAGS}
    ${LTTNG_LIBRARIES}
    ${GIO_LDFLAGS}
    ${FONTCONFIG_LDFLAGS}
    ${XKBCOMMON_LIBRARIES}

    ${CONTENT_HUB_LIBRARIES}

    Qt5::Core
    Qt5::DBus
    Qt5::Quick
    Qt5::Sensors
    Qt5::Qml
)

install(TARGETS qpa-mirserver LIBRARY DESTINATION "${QPA_PLUGIN_INSTALL_DIR}")
