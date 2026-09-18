# The response path uses admin-system's IAdminActions when it is installed, and degrades
# without it, so the dependency only orders the load.
voltmod_add_plugin(anticheat VERSION 1.1.0 OPTIONAL_DEPENDS admin-system)
target_link_libraries(anticheat PRIVATE cs2-contracts)

# Every SDK-free source lives under src/Detect/, so the test target never needs a hand-kept list.
# A TU there must not call anything defined in voltmod/src (Log, Time::Now, Random, Strings): the
# test target links VoltMod::Headers only, and the link step is what enforces the boundary.
file(GLOB_RECURSE anticheat_detect_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/Detect/*.cpp")
voltmod_add_tests(anticheat-tests SOURCES ${anticheat_detect_sources})

if(TARGET anticheat-tests)
    target_compile_definitions(anticheat-tests PRIVATE
        ANTICHEAT_DETECTIONS_JSONC="${CMAKE_CURRENT_SOURCE_DIR}/configs/detections.jsonc")
endif()
