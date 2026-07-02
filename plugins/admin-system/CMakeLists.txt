# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp inside cs2_add_plugin.
cs2_add_plugin(admin-system
    LIBRARIES
        libpqxx::pqxx
        nlohmann_json::nlohmann_json
)
