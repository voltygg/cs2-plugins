voltmod_add_plugin(anticheat)
target_link_libraries(anticheat PRIVATE cs2-contracts)

# src/Detect/ is SDK-free: the test target links VoltMod::Headers only, so a TU there that calls
# into voltmod/src (Log, Time::Now, Random, Strings) fails to link.
file(GLOB_RECURSE anticheat_detect_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/Detect/*.cpp")
voltmod_add_tests(anticheat-tests
    SOURCES ${anticheat_detect_sources}
    DEFINITIONS ANTICHEAT_DETECTIONS_JSONC="${CMAKE_CURRENT_SOURCE_DIR}/configs/detections.jsonc"
)
