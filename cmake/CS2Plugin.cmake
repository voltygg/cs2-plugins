include_guard(GLOBAL)

# Module dir at include time (CMAKE_CURRENT_LIST_DIR points at the caller inside a
# function); used to locate plugin.vdf.in. cs2kit_platform_arch comes from cs2-kit.
set(CS2_PLUGIN_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# Route all of `target_name`'s build artifacts to
# build/plugins/<name>/<platform_arch>/ with no rpath and no lib prefix.
function(cs2_set_output_dirs target_name platform_arch)
    set(output_dir "${CMAKE_BINARY_DIR}/plugins/${target_name}/${platform_arch}")

    # Ninja is single-config; per-config output-dir variants (VS/Xcode) aren't needed.
    set_target_properties("${target_name}" PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${target_name}"
        LIBRARY_OUTPUT_DIRECTORY "${output_dir}"
        RUNTIME_OUTPUT_DIRECTORY "${output_dir}"
        ARCHIVE_OUTPUT_DIRECTORY "${output_dir}"
        PDB_OUTPUT_DIRECTORY "${output_dir}"
        SKIP_BUILD_RPATH TRUE
        BUILD_RPATH ""
        INSTALL_RPATH ""
    )
endfunction()

# Create a Metamod plugin MODULE linked against CS2Kit, with output dirs and
# install rules. SOURCES defaults to a glob of src/*.cpp; INCLUDE_DIRS and
# LIBRARIES are appended to the defaults.
function(cs2_add_plugin target_name)
    cmake_parse_arguments(ARG "" "" "SOURCES;INCLUDE_DIRS;LIBRARIES" ${ARGN})

    if(NOT ARG_SOURCES)
        file(GLOB_RECURSE ARG_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        )
    endif()

    add_library("${target_name}" MODULE ${ARG_SOURCES})
    target_compile_features("${target_name}" PRIVATE cxx_std_23)

    target_sources("${target_name}" PRIVATE
        "${CS2KIT_HL2SDK_DIR}/public/tier0/memoverride.cpp"
        "${CS2KIT_HL2SDK_DIR}/tier1/convar.cpp"
    )

    target_include_directories("${target_name}" PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        ${ARG_INCLUDE_DIRS}
    )

    target_link_libraries("${target_name}" PRIVATE
        CS2Kit::CS2Kit
        ${ARG_LIBRARIES}
    )

    cs2kit_platform_arch(platform_arch)
    cs2_set_output_dirs("${target_name}" "${platform_arch}")

    cs2_install_plugin("${target_name}")
endfunction()

# install() rules for one plugin's deploy bundle, under a component named after the
# target. `cmake --install --component <target>` is the single source of the addons/
# layout consumed by local deploy and deploy/tools/cli.py package.
function(cs2_install_plugin target_name)
    if(WIN32)
        set(bin_subdir "win64")
    else()
        set(bin_subdir "linuxsteamrt64")
    endif()
    set(addon_bin "addons/${target_name}/bin/${bin_subdir}")

    # Only the loadable module ships (no ARCHIVE import .lib). COMPONENT must repeat
    # per kind: a MODULE's .dll/.so is the LIBRARY artifact, else it lands in the
    # default "Unspecified" component.
    install(TARGETS "${target_name}"
        LIBRARY DESTINATION "${addon_bin}" COMPONENT "${target_name}"
        RUNTIME DESTINATION "${addon_bin}" COMPONENT "${target_name}"
    )

    # Debug symbols when the build produced them (Release usually does not).
    if(WIN32)
        install(FILES "$<TARGET_PDB_FILE:${target_name}>"
            DESTINATION "${addon_bin}" COMPONENT "${target_name}" OPTIONAL)
    endif()

    # Generated, platform-correct VDF (the bin subdir is part of its "file" value).
    set(CS2_PLUGIN_NAME "${target_name}")
    set(CS2_PLUGIN_BIN_SUBDIR "${bin_subdir}")
    configure_file(
        "${CS2_PLUGIN_CMAKE_DIR}/plugin.vdf.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.vdf"
        @ONLY
        NEWLINE_STYLE LF
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.vdf"
        DESTINATION "addons/metamod" COMPONENT "${target_name}")

    # Configs except settings.jsonc (rendered per-server at deploy time).
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configs")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/configs/"
            DESTINATION "addons/${target_name}/configs"
            COMPONENT "${target_name}"
            PATTERN "settings.jsonc" EXCLUDE
        )
    endif()

    # Shared cs2-kit gamedata (location owned by cs2-kit, not hardcoded here).
    if(EXISTS "${CS2KIT_GAMEDATA_DIR}")
        install(DIRECTORY "${CS2KIT_GAMEDATA_DIR}/"
            DESTINATION "addons/cs2-kit/gamedata"
            COMPONENT "${target_name}")
    endif()
endfunction()
