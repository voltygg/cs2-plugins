# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp in cs2_add_plugin.
cs2_add_plugin(anticheat
    LIBRARIES
        nlohmann_json::nlohmann_json
)

# Detector logic is pure (free functions over settings + PlayerState), so it unit-tests
# without linking the SDK: recompile the SDK-free TUs against headers only.
if(BUILD_TESTING)
    cs2_add_tests(anticheat-tests
        SOURCES
            src/Detectors/AimSnapCore.cpp
            src/Detectors/SpinbotDetector.cpp
            src/Detectors/SilentAimDetector.cpp
            src/Detectors/ShotAngleDetector.cpp
            src/Detectors/AngleSanityDetector.cpp
            src/Detectors/NoFlashDetector.cpp
        INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
        LIBRARIES nlohmann_json::nlohmann_json
    )
endif()
