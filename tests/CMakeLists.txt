include_directories(
    include
)

find_package(GMock)

if(NOT WITH_MIR2)
    # TODO: fix for Mir2
    add_subdirectory(framework)
endif()
add_subdirectory(mirserver)
add_subdirectory(modules)

add_test(
    NAME requires-provides
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check-requires-provides.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
