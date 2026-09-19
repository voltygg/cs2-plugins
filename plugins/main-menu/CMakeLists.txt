voltmod_add_plugin(main-menu)
target_link_libraries(main-menu PRIVATE cs2-contracts)

voltmod_add_tests(main-menu-tests
    SOURCES
        src/Config.cpp
)
