include_guard(GLOBAL)

function(cs2_platform_arch out_var)
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Only x86_64 builds are supported.")
    endif()

    if(WIN32)
        set("${out_var}" "windows-x86_64" PARENT_SCOPE)
    elseif(UNIX)
        set("${out_var}" "linux-x86_64" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Only Windows and Linux builds are supported.")
    endif()
endfunction()

function(cs2_set_output_dirs target_name platform_arch)
    set(output_dir "${CMAKE_BINARY_DIR}/plugins/${target_name}/${platform_arch}")

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

    foreach(config IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_target_properties("${target_name}" PROPERTIES
            "LIBRARY_OUTPUT_DIRECTORY_${config}" "${output_dir}"
            "RUNTIME_OUTPUT_DIRECTORY_${config}" "${output_dir}"
            "ARCHIVE_OUTPUT_DIRECTORY_${config}" "${output_dir}"
            "PDB_OUTPUT_DIRECTORY_${config}" "${output_dir}"
        )
    endforeach()
endfunction()

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

    if(WIN32)
        target_link_libraries("${target_name}" PRIVATE
            kernel32
            user32
            gdi32
            winspool
            comdlg32
            advapi32
            shell32
            ole32
            oleaut32
            uuid
            odbc32
            odbccp32
            ws2_32
            crypt32
            secur32
            wldap32
            iphlpapi
        )

        target_link_options("${target_name}" PRIVATE /SUBSYSTEM:WINDOWS)
    endif()

    cs2_platform_arch(platform_arch)
    cs2_set_output_dirs("${target_name}" "${platform_arch}")
endfunction()
