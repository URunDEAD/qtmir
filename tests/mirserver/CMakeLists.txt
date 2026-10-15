add_subdirectory(EventBuilder)
add_subdirectory(QtEventFeeder)
add_subdirectory(URLBuilderDispatcher)
add_subdirectory(Screen)
if(NOT WITH_MIR2)
    # TODO: fix for Mir2
    add_subdirectory(ScreensModel)
    # Relevant for Mir1 only
    add_subdirectory(miral)
endif()
