# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp in cs2_add_plugin.
cs2_add_plugin(anticheat
    LIBRARIES
        nlohmann_json::nlohmann_json
)

# Detector logic is pure (free functions over settings + PlayerState), so it unit-tests
# without linking the SDK: recompile the SDK-free TUs against headers only.
if(BUILD_TESTING)
    file(GLOB ANTICHEAT_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    add_executable(anticheat-tests
        ${ANTICHEAT_TEST_SOURCES}
        "${CMAKE_SOURCE_DIR}/vendor/cs2-kit/tests/MicroTestMain.cpp"
        src/Detectors/AimSnapCore.cpp
        src/Detectors/SpinbotDetector.cpp
        src/Detectors/SilentAimDetector.cpp
        src/Detectors/ShotAngleDetector.cpp
        src/Detectors/AngleSanityDetector.cpp
        src/Detectors/NoFlashDetector.cpp
    )
    target_compile_features(anticheat-tests PRIVATE cxx_std_23)
    target_include_directories(anticheat-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/vendor/cs2-kit/include"
        "${CMAKE_SOURCE_DIR}/vendor/cs2-kit/tests"
    )
    target_link_libraries(anticheat-tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME anticheat COMMAND anticheat-tests)
endif()
