cmake_minimum_required(VERSION 3.1)

project(qtmir)

set(QTMIR_VERSION_MAJOR 0)
set(QTMIR_VERSION_MINOR 8)
set(QTMIR_VERSION_PATCH 0)

set(QTMIR_VERSION ${QTMIR_VERSION_MAJOR}.${QTMIR_VERSION_MINOR}.${QTMIR_VERSION_PATCH})

if(${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
   message(FATAL_ERROR "In-tree build attempt detected, aborting. Set your build dir outside your source dir, delete CMakeCache.txt and CMakeFiles/ from source root and try again.")
endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package (ECM 1.7.0 QUIET NO_MODULE)
if (ECM_FOUND)
    # Provides us with -DECM_ENABLE_SANITIZERS='X'
    # Where X can be address, thread, memory, leak, undefined
    include("${ECM_MODULE_DIR}/ECMEnableSanitizers.cmake")
endif()

# add custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -fno-strict-aliasing -Wextra")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

set (CMAKE_CXX_STANDARD 20)

if ("${CMAKE_CXX_COMPILER}" MATCHES "clang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type-c-linkage -Wno-mismatched-tags -Wno-inconsistent-missing-override -Wno-implicit-exception-spec-mismatch -Wno-unknown-pragmas")
endif()


#find_package(CoverageReport)
#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage (or ninja if you use that backend)
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
if(cmake_build_type_lower MATCHES coverage)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage" )
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --coverage" )
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage" )
    ENABLE_COVERAGE_REPORT(TARGETS ${SHELL_APP} FILTER /usr/include ${CMAKE_SOURCE_DIR}/tests/* ${CMAKE_BINARY_DIR}/*)
endif()

# Make sure we have all the needed symbols
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,defs")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,defs")

# Static C++ checks
add_custom_target(cppcheck COMMAND cppcheck --enable=all -q --error-exitcode=2
                                       ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)

option(WITH_CONTENTHUB "Build with lomiri-content-hub" OFF)
option(WITH_VALGRIND "Build with Valgrind detection" ON)
option(WITH_MIR2 "Build with mir 2.x" OFF)

include(FindPkgConfig)
find_package(Qt5Core 5.9 REQUIRED)
find_package(Qt5DBus 5.9 REQUIRED)
find_package(Qt5Gui 5.9 REQUIRED)
find_package(Qt5Qml 5.9 REQUIRED)
find_package(Qt5Quick 5.9 REQUIRED)
find_package(Qt5Sensors 5.9 REQUIRED)
find_package(Qt5Test 5.9 REQUIRED)

find_package(Threads REQUIRED)

pkg_check_modules(MIRSERVER mirserver>=0.26 REQUIRED)
if(WITH_MIR2)
pkg_check_modules(MIROIL miroil REQUIRED)
else()
pkg_check_modules(MIRCLIENT mirclient>=0.26 REQUIRED)
endif()
pkg_check_modules(MIRRENDERERGLDEV mir-renderer-gl-dev>=0.26 REQUIRED)
pkg_check_modules(MIRAL miral>=1.1.0 REQUIRED)
pkg_check_modules(MIRWAYLAND mirwayland REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)

pkg_check_modules(XKBCOMMON xkbcommon REQUIRED)
pkg_check_modules(GLIB glib-2.0 REQUIRED)
pkg_check_modules(PROCESS_CPP process-cpp REQUIRED)
pkg_check_modules(LOMIRI_APP_LAUNCH lomiri-app-launch-0 REQUIRED)
pkg_check_modules(URL_DISPATCHER lomiri-url-dispatcher REQUIRED)
pkg_check_modules(EGL egl)
pkg_check_modules(GIO gio-2.0)
pkg_check_modules(GIO_UNIX gio-unix-2.0)
pkg_check_modules(LTTNG lttng-ust)
pkg_check_modules(GSETTINGS_QT REQUIRED gsettings-qt)
pkg_check_modules(QTDBUSTEST libqtdbustest-1 REQUIRED)
pkg_check_modules(QTDBUSMOCK libqtdbusmock-1 REQUIRED)
pkg_check_modules(APPLICATION_API REQUIRED lomiri-shell-application=28)

if(WITH_VALGRIND)
    pkg_check_modules(VALGRIND valgrind REQUIRED)
    add_definitions(-DWITH_VALGRIND)
endif()

if(WITH_CONTENTHUB)
pkg_check_modules(CONTENT_HUB liblomiri-content-hub>=1.0.0 REQUIRED)
message(STATUS "Bulding with lomiri-content-hub")
endif()

include_directories(SYSTEM ${APPLICATION_API_INCLUDE_DIRS})

# Disable all deprecated qt functions in and before qt version 5.9
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050900)

# Use the fast string builder
add_definitions(-DQT_USE_QSTRINGBUILDER)

if (DEFINED USE_OPENGLES OR DEFINED USE_OPENGL_BUT_LINK_AGAINST_OPENGLES)
    message(DEPRECATION "USE_OPENGLES and USE_OPENGL_BUT_LINK_AGAINST_OPENGLES is no longer used as QtMir no longer links directly to libGL or libGLES.")
endif ()

# Standard install paths
include(GNUInstallDirs)


# Determine QPA plugin install path
# FIXME(greyback) there must be a better way than calling a private macro to set a property.
# I'm manually calling a macro from Qt5GuiConfig.cmake, but I don't understand why it isn't being called.
if (Qt5Gui_VERSION VERSION_LESS 5.14.0)
  _populate_Gui_plugin_properties("Gui" "PLATFORMS" "platforms")
else()
  _populate_Gui_plugin_properties("Gui" "PLATFORMS" "platforms" FALSE)
endif()
get_target_property(Qt5Gui_QPA_Plugin_Path Qt5::Gui IMPORTED_LOCATION_PLATFORMS)

# Set QML module install path
set(QML_MODULE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/qt5/qml")
set(QTMIR_DATA_DIR ${CMAKE_INSTALL_DATADIR}/qtmir)


# Customisations for the build type
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower)

if(cmake_build_type_lower MATCHES relwithdebinfo) # workaround for http://public.kitware.com/Bug/view.php?id=14696
    add_definitions(-DQT_NO_DEBUG)
endif()
if ("${CMAKE_BUILD_TYPE}" STREQUAL "release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "relwithdebinfo")
    option(Werror "Treat warnings as errors" ON)
else()
    option(Werror "Treat warnings as errors" OFF)
endif()

if (Werror)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()


# Mir uses boost, which does not like Qt defining macros named "signals" and "slots"
add_definitions(-DQT_NO_KEYWORDS)

# Tests
if (NO_TESTS OR WITH_MIR2)
    message(STATUS "Tests disabled")
else()
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

# add subdirectories to build
add_subdirectory(src)
add_subdirectory(demos)
add_subdirectory(benchmarks)
add_subdirectory(data)
