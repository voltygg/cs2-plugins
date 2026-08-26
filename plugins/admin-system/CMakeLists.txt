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

# Recompile the SDK-free cores into the test binary.
if(BUILD_TESTING)
    voltmod_add_tests(admin-system-tests
        SOURCES
            src/Maps/MapQuery.cpp
            src/Maps/VoteMath.cpp
            src/Weapons/WeaponCatalog.cpp
        INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
endif()
