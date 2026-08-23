# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp in cs2_add_plugin.
cs2_add_plugin(bhop
    DESCRIPTION "Bunny-hop movement tuning"
    LIBRARIES
        nlohmann_json::nlohmann_json
)
