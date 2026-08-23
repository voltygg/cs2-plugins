cmake_minimum_required(VERSION 4.3)

project(cs2-plugins VERSION 1.0.0 LANGUAGES CXX)

include(CTest)

# CS2Kit::CS2Kit plus cs2_add_plugin/cs2_add_tests, and the HL2SDK/Metamod packages
# behind them. Postgres is on via cs2-kit/*:with_postgres in conanfile.py.
find_package(cs2-kit CONFIG REQUIRED)

# Must precede the plugins that link it.
add_subdirectory(plugins/contracts)

add_subdirectory(plugins/admin-system)
add_subdirectory(plugins/bhop)
add_subdirectory(plugins/anticheat)
