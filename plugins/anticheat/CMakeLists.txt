voltmod_add_plugin(anticheat
    VERSION 1.0.0
    DESCRIPTION "Behavioural anticheat: detection cores, correlation and response"
    LIBRARIES
        cs2-contracts
        glaze::glaze
)

# Recompile SDK-free detector cores into the test binary.
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
            # Validation only; the file-reading half lives in DetectionDataManager.cpp.
            src/Core/DetectionData.cpp
        INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
        LIBRARIES glaze::glaze
    )

    # Fail tests when the shipped operator-editable table cannot be parsed.
    target_compile_definitions(anticheat-tests PRIVATE
        ANTICHEAT_DETECTIONS_JSONC="${CMAKE_CURRENT_SOURCE_DIR}/configs/detections.jsonc")
endif()
