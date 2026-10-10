set(
  MIRAL_TEST_SOURCES
  edid_test.cpp
)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

include_directories(
    SYSTEM
    ${MIROIL_INCLUDE_DIRS}
)

add_executable(MirALTests ${MIRAL_TEST_SOURCES})

target_link_libraries(MirALTests
  qtmirserver
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES}
)

add_test(MirAL, MirALTests)
