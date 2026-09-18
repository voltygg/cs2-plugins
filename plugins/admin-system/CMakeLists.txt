voltmod_add_plugin(admin-system FEATURES DATABASE)
target_link_libraries(admin-system PRIVATE cs2-contracts)

# Recompile the SDK-free cores into the test binary. DATABASE lets the schema tests run the
# migrations against in-memory SQLite.
voltmod_add_tests(admin-system-tests
    FEATURES DATABASE
    SOURCES
        src/Database/JsonList.cpp
        src/Maps/MapQuery.cpp
        src/Maps/VoteMath.cpp
        src/Weapons/WeaponCatalog.cpp
    DEFINITIONS
        ADMIN_SYSTEM_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
