# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp in voltmod_add_plugin.
voltmod_add_plugin(anticheat
    DESCRIPTION "Behavioural anticheat: detection cores, correlation and response"
    # Advisory: without it the detectors still run, but bans and alerts have nowhere to go.
    DEPENDS "admin-system>=1.0"
    LIBRARIES
        cs2-contracts
        nlohmann_json::nlohmann_json
)

# Every detection module is an SDK-free core over plain sample structs, so the whole detection
# layer unit-tests without linking the SDK: recompile those TUs against headers only.
if(BUILD_TESTING)
    voltmod_add_tests(anticheat-tests
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

    # The shipped detection tables are data an operator edits, so parse them at build time rather
    # than finding out from a warning in a server log.
    target_compile_definitions(anticheat-tests PRIVATE
        ANTICHEAT_DETECTIONS_JSONC="${CMAKE_CURRENT_SOURCE_DIR}/configs/detections.jsonc")
endif()
