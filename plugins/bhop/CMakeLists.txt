voltmod_add_plugin(bhop
    VERSION 1.0.0
    DESCRIPTION "Bunny-hop movement tuning"
    LIBRARIES
        glaze::glaze
)

# The settings model only; unknown keys are errors, so a stale key in the shipped file has to fail
# here rather than at server load.
if(BUILD_TESTING)
    voltmod_add_tests(bhop-tests
        INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
        LIBRARIES glaze::glaze
    )

    target_compile_definitions(bhop-tests PRIVATE
        BHOP_SETTINGS_JSONC="${CMAKE_CURRENT_SOURCE_DIR}/configs/settings.jsonc")
endif()
