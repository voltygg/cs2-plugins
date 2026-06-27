include_guard(GLOBAL)

# Reuses cs2kit_pick_target / cs2kit_alias_interface (the root includes
# CS2KitThirdParty before this runs).

function(cs2_configure_third_party)
    # cpr/nlohmann_json come from cs2-kit; libpqxx is plugin-only, resolved here.
    find_package(libpqxx CONFIG REQUIRED)

    cs2kit_pick_target(CS2_LIBPQXX_TARGET
        libpqxx::pqxx
        libpqxx::libpqxx
        pqxx
        libpqxx
    )

    cs2kit_alias_interface(thirdparty::libpqxx "${CS2_LIBPQXX_TARGET}")
endfunction()
