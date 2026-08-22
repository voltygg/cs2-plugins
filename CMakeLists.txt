cmake_minimum_required(VERSION 4.3)

project(cs2-plugins VERSION 1.0.0 LANGUAGES CXX)

include(CTest)

# cpr/nlohmann_json/libpqxx are all found inside cs2-kit; Postgres support is
# opt-in there, and this monorepo's plugins use it.
set(CS2KIT_ENABLE_POSTGRES ON)

add_subdirectory(vendor/cs2-kit)

# Must precede the plugins that link it.
add_subdirectory(plugins/contracts)

add_subdirectory(plugins/admin-system)
add_subdirectory(plugins/bhop)
add_subdirectory(plugins/anticheat)
