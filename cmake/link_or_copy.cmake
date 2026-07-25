include_guard(GLOBAL)


# ==================================================
# Directory Deployment
# ==================================================

# Copies source files only when their contents change.
function(_copy_directory_if_different
         source_directory destination_directory)
    file(MAKE_DIRECTORY "${destination_directory}")
    file(GLOB_RECURSE source_files
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        "${source_directory}/*"
    )

    foreach(source_file IN LISTS source_files)
        file(RELATIVE_PATH relative_path
            "${source_directory}" "${source_file}"
        )
        set(destination_file
            "${destination_directory}/${relative_path}"
        )
        get_filename_component(destination_parent
            "${destination_file}" DIRECTORY
        )
        file(MAKE_DIRECTORY "${destination_parent}")

        # COPYONLY preserves binary data; unchanged outputs keep their timestamp.
        configure_file(
            "${source_file}" "${destination_file}" COPYONLY
        )
    endforeach()
endfunction()

# Links one directory when supported; otherwise copies changed files.
function(link_or_copy_directory
         source_directory destination_directory)
    if(NOT IS_DIRECTORY "${source_directory}")
        message(FATAL_ERROR
            "Directory source does not exist: ${source_directory}"
        )
    endif()

    file(REAL_PATH "${source_directory}" source_real_path)

    # Keep an existing link only when it already targets the source.
    if(IS_SYMLINK "${destination_directory}")
        file(REAL_PATH "${destination_directory}" destination_real_path)
        if(source_real_path STREQUAL destination_real_path)
            message(STATUS
                "Directory link is current: ${destination_directory}"
            )
            return()
        endif()

        message(FATAL_ERROR
            "Directory link points to a different source: "
            "${destination_directory}"
        )
    endif()

    # Test the platform capability instead of assuming it from the OS name.
    if(NOT EXISTS "${destination_directory}")
        get_filename_component(destination_parent
            "${destination_directory}" DIRECTORY
        )
        file(MAKE_DIRECTORY "${destination_parent}")
        file(CREATE_LINK
            "${source_real_path}"
            "${destination_directory}"
            SYMBOLIC
            RESULT link_result
        )

        if(link_result STREQUAL "0")
            message(STATUS
                "Directory linked: ${destination_directory}"
            )
            return()
        endif()

        message(STATUS
            "Directory link unavailable; copying changed files instead: "
            "${link_result}"
        )
    elseif(NOT IS_DIRECTORY "${destination_directory}")
        message(FATAL_ERROR
            "Directory destination is not a directory: "
            "${destination_directory}"
        )
    endif()

    _copy_directory_if_different(
        "${source_real_path}" "${destination_directory}"
    )
    message(STATUS
        "Directory contents synchronized: ${destination_directory}"
    )
endfunction()
