voltmod_add_plugin(admin-system DATABASE)
target_link_libraries(admin-system PRIVATE cs2-contracts)

# SDK-free cores; DATABASE lets the schema tests run the migrations on in-memory SQLite.
voltmod_add_tests(admin-system-tests
    DATABASE
    SOURCES
        src/Admin/Effects/EffectManager.cpp
        src/Database/JsonList.cpp
        src/Maps/MapQuery.cpp
        src/Maps/VoteMath.cpp
        src/Weapons/WeaponCatalog.cpp
    DEFINITIONS
        ADMIN_SYSTEM_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
