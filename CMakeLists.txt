cmake_minimum_required(VERSION 4.3)

# version.txt is the one place this repo's version is written; conanfile.py reads the
# same file, and voltmod_add_plugin stamps it into every plugin manifest.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" CS2_PLUGINS_VERSION)
string(STRIP "${CS2_PLUGINS_VERSION}" CS2_PLUGINS_VERSION)

project(cs2-plugins VERSION ${CS2_PLUGINS_VERSION} LANGUAGES CXX)

include(CTest)

# VoltMod::VoltMod plus voltmod_add_plugin/voltmod_add_tests, and the HL2SDK/Metamod packages
# behind them. Postgres is on via voltmod/*:with_postgres in conanfile.py.
find_package(voltmod CONFIG REQUIRED)

# Must precede the plugins that link it.
add_subdirectory(plugins/contracts)

add_subdirectory(plugins/admin-system)
add_subdirectory(plugins/bhop)
add_subdirectory(plugins/anticheat)
