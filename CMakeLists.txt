cmake_minimum_required(VERSION 4.3)

project(cs2-plugins LANGUAGES CXX)

include(CTest)

# VoltMod::VoltMod plus voltmod_add_plugin/voltmod_add_tests, and the HL2SDK/Metamod packages
# behind them. Postgres is on via voltmod/*:with_postgres in conanfile.py.
find_package(voltmod CONFIG REQUIRED)

# Must precede the plugins that link it.
add_subdirectory(plugins/contracts)

add_subdirectory(plugins/admin-system)
add_subdirectory(plugins/bhop)
add_subdirectory(plugins/anticheat)
