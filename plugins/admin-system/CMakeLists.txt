# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp inside voltmod_add_plugin.
# pqxx joins the PCH here because App.hpp (via Database/Api.hpp) pulls it
# into most of this plugin's TUs; it is deliberately not in the framework-wide PCH.
voltmod_add_plugin(admin-system
    VERSION 1.0.0
    DESCRIPTION "Admin groups, punishments, reports and the fun-command menu"
    FEATURES DATABASE
    LIBRARIES
        cs2-contracts
        libpqxx::pqxx
        nlohmann_json::nlohmann_json
    PCH_HEADERS
        "<pqxx/pqxx>"
)
