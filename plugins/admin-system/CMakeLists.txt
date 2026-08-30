voltmod_add_plugin(admin-system VERSION 1.0.0 FEATURES DATABASE)
target_link_libraries(admin-system PRIVATE cs2-contracts)

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
