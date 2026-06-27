include_guard(GLOBAL)

function(cs2_pick_target out_var)
    foreach(candidate IN LISTS ARGN)
        if(TARGET "${candidate}")
            set("${out_var}" "${candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    message(FATAL_ERROR "None of the expected CMake targets exist: ${ARGN}")
endfunction()

function(cs2_alias_interface alias_name target_name)
    string(REPLACE "::" "_" local_name "${alias_name}")

    if(NOT TARGET "${alias_name}")
        add_library("${local_name}" INTERFACE)
        add_library("${alias_name}" ALIAS "${local_name}")
        target_link_libraries("${local_name}" INTERFACE "${target_name}")
    endif()
endfunction()

function(cs2_configure_third_party)
    # cpr and nlohmann_json are owned by cs2-kit (add_subdirectory below creates
    # thirdparty::cpr / thirdparty::nlohmann_json). libpqxx is unique to the plugins.
    find_package(libpqxx CONFIG REQUIRED)

    cs2_pick_target(CS2_LIBPQXX_TARGET
        libpqxx::pqxx
        libpqxx::libpqxx
        pqxx
        libpqxx
    )

    cs2_alias_interface(thirdparty::libpqxx "${CS2_LIBPQXX_TARGET}")
endfunction()
