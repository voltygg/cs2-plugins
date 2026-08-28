cmake_minimum_required(VERSION 4.3)

project(cs2-plugins LANGUAGES CXX)

include(CTest)

# Provides VoltMod targets and plugin/test helpers. Conan enables PostgreSQL.
find_package(voltmod CONFIG REQUIRED)

# Plugin targets link this contract library.
add_subdirectory(plugins/contracts)

add_subdirectory(plugins/admin-system)
add_subdirectory(plugins/bhop)
add_subdirectory(plugins/anticheat)
add_subdirectory(plugins/ui-lab)
