# `Database/Api.hpp` brings pqxx into most translation units, so precompile it
# for this plugin without adding it to VoltMod's global PCH.
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
