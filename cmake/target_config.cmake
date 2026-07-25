include_guard(GLOBAL)


# ==================================================
# Target Configuration
# ==================================================

# Applies named option lists to one target.
# Keys: COMMON, DEBUG, RELEASE, RELWITHDEBINFO, DEBUG_DEFINITIONS.
function(target_apply_build_options target_name)
    cmake_parse_arguments(
        BUILD_OPTIONS
        ""
        ""
        "COMMON;DEBUG;RELEASE;RELWITHDEBINFO;DEBUG_DEFINITIONS"
        ${ARGN}
    )
    target_compile_options(${target_name} PRIVATE
        ${BUILD_OPTIONS_COMMON}
        $<$<CONFIG:Debug>:${BUILD_OPTIONS_DEBUG}>
        $<$<CONFIG:Release>:${BUILD_OPTIONS_RELEASE}>
        $<$<CONFIG:RelWithDebInfo>:${BUILD_OPTIONS_RELWITHDEBINFO}>
    )
    target_compile_definitions(${target_name} PRIVATE
        $<$<CONFIG:Debug>:${BUILD_OPTIONS_DEBUG_DEFINITIONS}>
    )
endfunction()

# Normalizes target output paths across single- and multi-config generators.
function(target_set_output_directory
         target_name output_kind output_directory)
    set_target_properties(${target_name} PROPERTIES
        "${output_kind}_OUTPUT_DIRECTORY" "${output_directory}"
    )
    foreach(configuration DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_target_properties(${target_name} PROPERTIES
            "${output_kind}_OUTPUT_DIRECTORY_${configuration}"
            "${output_directory}"
        )
    endforeach()
endfunction()
