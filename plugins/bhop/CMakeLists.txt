# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp in cs2_add_plugin.
cs2_add_plugin(bhop
    DESCRIPTION "Bunny-hop movement tuning"
    # No database here, so no libpqxx on the link line. App pulls the rest.
    COMPONENTS App
    LIBRARIES
        nlohmann_json::nlohmann_json
)
