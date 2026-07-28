# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp in cs2_add_plugin.
cs2_add_plugin(anticheat
    LIBRARIES
        nlohmann_json::nlohmann_json
)

# Every detection module is an SDK-free core over plain sample structs, so the whole detection
# layer unit-tests without linking the SDK: recompile those TUs against headers only.
if(BUILD_TESTING)
    cs2_add_tests(anticheat-tests
        SOURCES
            src/Core/WeaponClass.cpp
            src/Correlation/ShotCorrelatorCore.cpp
            src/Detectors/AimbotCore.cpp
            src/Detectors/AimlockCore.cpp
            src/Detectors/AntiAimCore.cpp
            src/Detectors/AntiAimMotionCore.cpp
            src/Detectors/SilentAimCore.cpp
            src/Detectors/NamechangerCore.cpp
            src/Detectors/InvalidCvarRules.cpp
        INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
        LIBRARIES nlohmann_json::nlohmann_json
    )
endif()
