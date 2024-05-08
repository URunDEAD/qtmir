include(UseLttngGenTp)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/platforms/mirserver
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_BINARY_DIR} # for tracepoints.h
)

include_directories(
    SYSTEM
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${GIO_UNIX_INCLUDE_DIRS}
    ${MIRAL_INCLUDE_DIRS}
    ${MIRSERVER_INCLUDE_DIRS}
    ${MIRRENDERERGLDEV_INCLUDE_DIRS}
    ${PROCESS_CPP_INCLUDE_DIRS}
    ${UBUNTU_PLATFORM_API_INCLUDE_DIRS}
    ${LOMIRI_APP_LAUNCH_INCLUDE_DIRS}
    ${GSETTINGS_QT_INCLUDE_DIRS}

    ${LTTNG_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt5Qml_PRIVATE_INCLUDE_DIRS}
    ${Qt5Quick_PRIVATE_INCLUDE_DIRS}
)

# We have to remove -pedantic in order to compile tracepoints.c
string (REPLACE " -pedantic " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
# Needed to compile tracepoints in C99 mode.
add_definitions(-DBYTE_ORDER=__BYTE_ORDER)

# Generate tracepoints.c and .h from tracepoints.tp
add_lttng_gen_tp(NAME tracepoints)
add_custom_target(QtMirApplication_LTTNG
    DEPENDS tracepoints.h tracepoints.c
)

set(QMLMIRPLUGIN_SRC
    application_manager.cpp
    application.cpp
    compositortextureprovider.cpp
    ../../../common/abstractdbusservicemonitor.cpp
    ../../../common/debughelpers.cpp
    dbusfocusinfo.cpp
    plugin.cpp
    mirsurface.cpp
    mirsurfaceinterface.h
    mirsurfaceitem.cpp
    mirsurfacelistmodel.cpp
    mirbuffersgtexture.cpp
    proc_info.cpp
    session.cpp
    sharedwakelock.cpp
    surfacemanager.cpp
    taskcontroller.cpp
    lal/applicationinfo.cpp
    lal/taskcontroller.cpp
    timer.cpp
    timesource.cpp
    tracepoints.c
    settings.cpp
    windowmodel.cpp
# We need to run moc on these headers
    ${APPLICATION_API_INCLUDEDIR}/lomiri/shell/application/ApplicationInfoInterface.h
    ${APPLICATION_API_INCLUDEDIR}/lomiri/shell/application/ApplicationManagerInterface.h
    ${APPLICATION_API_INCLUDEDIR}/lomiri/shell/application/Mir.h
    ${APPLICATION_API_INCLUDEDIR}/lomiri/shell/application/MirSurfaceInterface.h
    ${APPLICATION_API_INCLUDEDIR}/lomiri/shell/application/MirSurfaceItemInterface.h
    ${APPLICATION_API_INCLUDEDIR}/lomiri/shell/application/MirSurfaceListInterface.h
    ${APPLICATION_API_INCLUDEDIR}/lomiri/shell/application/SurfaceManagerInterface.h
# Feed the automoc monster
    session_interface.h
    applicationinfo.h
    taskcontroller.h
    settings_interface.h
)

add_library(qtmirapplicationplugin SHARED
    ${QMLMIRPLUGIN_SRC}
)
add_dependencies(qtmirapplicationplugin QtMirApplication_LTTNG)

target_compile_definitions(qtmirapplicationplugin
    PRIVATE
    -DMIR_VERSION_MAJOR=${MIR_VERSION_MAJOR}
)

# Frig for files that still rely on mirserver-dev
string(REPLACE ";" " -I" QTMIR_ADD_MIRSERVER "-I ${MIRSERVER_INCLUDE_DIRS}")
set_source_files_properties(mirsurface.cpp         PROPERTIES COMPILE_FLAGS "${CMAKE_CXXFLAGS} ${QTMIR_ADD_MIRSERVER}")

target_link_libraries(
    qtmirapplicationplugin

    ${CMAKE_THREAD_LIBS_INIT}

    ${GLIB_LDFLAGS}
    ${UBUNTU_PLATFORM_API_LDFLAGS}
    ${MIRSERVER_LDFLAGS}
    ${PROCESS_CPP_LDFLAGS}
    ${LOMIRI_APP_LAUNCH_LDFLAGS}
    ${LTTNG_LDFLAGS}
    ${GSETTINGS_QT_LDFLAGS}

    Qt5::Core
    Qt5::DBus
    Qt5::Qml
    Qt5::Quick

    qtmirserver
)


# install
set(QML_VERSION "0.${MIR_VERSION_MAJOR}")
add_qml_plugin(QtMir.Application ${QML_VERSION} QtMir.${QML_VERSION}/Application TARGETS qtmirapplicationplugin)
install(FILES com.canonical.qtmir.gschema.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas)
