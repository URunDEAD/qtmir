add_subdirectory(General)
add_subdirectory(SharedWakelock)

if(NOT WITH_MIR2)
    # TODO: fix for Mir2
    add_subdirectory(Application)
    add_subdirectory(ApplicationManager)
    add_subdirectory(Session)
    add_subdirectory(SurfaceManager)
    add_subdirectory(WindowManager)
endif()
