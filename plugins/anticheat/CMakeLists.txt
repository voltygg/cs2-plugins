voltmod_add_plugin(anticheat VERSION 1.1.0)
target_link_libraries(anticheat PRIVATE cs2-contracts)

# Recompile SDK-free detector cores into the test binary.
voltmod_add_tests(anticheat-tests
    SOURCES
        src/Core/WeaponClass.cpp
        src/Core/DetectionData.cpp
        src/Correlation/ShotCorrelatorCore.cpp
        src/Aim/AimbotCore.cpp
        src/Aim/AimlockCore.cpp
        src/Aim/AntiAimCore.cpp
        src/Aim/AntiAimMotionCore.cpp
        src/Aim/SilentAimCore.cpp
        src/Aim/TriggerbotCore.cpp
        src/Aim/RecoilCore.cpp
        src/Aim/MouseCore.cpp
        src/Aim/WallhackCore.cpp
        src/Client/NamechangerCore.cpp
        src/Client/InvalidCvarRules.cpp
)

# Fail tests when the shipped operator-editable table cannot be parsed.
if(TARGET anticheat-tests)
    target_compile_definitions(anticheat-tests PRIVATE
        ANTICHEAT_DETECTIONS_JSONC="${CMAKE_CURRENT_SOURCE_DIR}/configs/detections.jsonc")
endif()
