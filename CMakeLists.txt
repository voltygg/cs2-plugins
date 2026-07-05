cmake_minimum_required(VERSION 4.3)

if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Use ccache when available; before project() so it propagates to subdirectories.
if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    endif()
endif()

project(cs2-plugins VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Conan imported targets are directory-scoped; make them visible across the
# plugin directories.
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CTest)

# cpr/nlohmann_json/libpqxx are all found inside cs2-kit; Postgres support is
# opt-in there, and this monorepo's plugins use it.
set(CS2KIT_ENABLE_POSTGRES ON)

add_subdirectory(vendor/cs2-kit)

include(CS2Plugin)

add_subdirectory(plugins/admin-system)
add_subdirectory(plugins/bhop)
