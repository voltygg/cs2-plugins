# Sources default to a CONFIGURE_DEPENDS glob of src/*.cpp inside cs2_add_plugin.
# pqxx joins the PCH here because Managers.hpp (via Database/Api.hpp) pulls it
# into most of this plugin's TUs; it is deliberately not in the kit-wide PCH.
cs2_add_plugin(admin-system
    LIBRARIES
        cs2-contracts
        libpqxx::pqxx
        nlohmann_json::nlohmann_json
    PCH_HEADERS
        "<pqxx/pqxx>"
)
