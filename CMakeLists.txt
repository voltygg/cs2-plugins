cmake_minimum_required(VERSION 4.3)

project(cs2-plugins LANGUAGES CXX)

include(CTest)

find_package(voltmod CONFIG REQUIRED)

add_subdirectory(plugins/contracts)

add_subdirectory(plugins/admin-system)
add_subdirectory(plugins/bhop)
add_subdirectory(plugins/anticheat)
