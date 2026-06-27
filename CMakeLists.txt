cmake_minimum_required(VERSION 4.3)

if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(cs2-plugins VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CTest)
include(ThirdParty)

cs2_configure_third_party()

add_subdirectory(vendor/cs2-kit)

include(CS2Plugin)

add_subdirectory(plugins/admin-system)

if(BUILD_TESTING)
    add_executable(cs2kit-utils-tests
        tests/MicroTestMain.cpp
        tests/ParseDurationTests.cpp
        tests/StringUtilsTests.cpp
        tests/SteamIdTests.cpp
        tests/TimeUtilsTests.cpp
        vendor/cs2-kit/src/Utils/StringUtils.cpp
        vendor/cs2-kit/src/Utils/SteamId.cpp
        vendor/cs2-kit/src/Utils/TimeUtils.cpp
    )

    target_compile_features(cs2kit-utils-tests PRIVATE cxx_std_23)
    target_include_directories(cs2kit-utils-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/cs2-kit/include"
    )

    add_test(NAME cs2kit-utils COMMAND cs2kit-utils-tests)
endif()
