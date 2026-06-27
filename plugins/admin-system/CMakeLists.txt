file(GLOB_RECURSE ADMIN_SYSTEM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

cs2_add_plugin(admin-system
    SOURCES ${ADMIN_SYSTEM_SOURCES}
    LIBRARIES
        thirdparty::libpqxx
        thirdparty::nlohmann_json
)
