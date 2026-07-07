# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp in cs2_add_plugin.
cs2_add_plugin(anticheat
    LIBRARIES
        nlohmann_json::nlohmann_json
)
