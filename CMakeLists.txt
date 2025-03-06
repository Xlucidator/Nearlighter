
cmake_minimum_required(VERSION 3.12)

project(Nearlighter)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)


# Build Mode Select
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()


# Pre-define
add_compile_definitions("$<$<CONFIG:Debug>:DEBUG>")


# Compile Options
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# function define
## smart copy files in file_list from ${CMAKE_SOURCE_DIR} to {CMAKE_BIN_DIR}
function(smart_copy_files file_list flattened)
    set(copy_src_list "")
    set(copy_dst_list "")
    foreach(src_file IN LISTS file_list)
        # prepare paths
        if (flattened)  # file -> ${CMAKE_BINARY_DIR}/file
            get_filename_component(rel_path "${src_file}" NAME)
        else()          # ${CMAKE_SOURCE_DIR}/rel_path -> ${CMAKE_BIN_DIR}/rel_path
            file(RELATIVE_PATH rel_path "${CMAKE_SOURCE_DIR}" "${src_file}")
            #message(STATUS "    rel_path = ${rel_path}")
        endif()
        set(dst_path "${CMAKE_BINARY_DIR}/${rel_path}")         # dst_path
        get_filename_component(dst_dir "${dst_path}" DIRECTORY) # dst_dir
        file(MAKE_DIRECTORY "${dst_dir}") # ensure dst_dir exists

        # check if file needs copy
        set(need_copy FALSE)
        if(NOT EXISTS "${dst_path}")
            set(need_copy TRUE)
        else()
            file(TIMESTAMP "${src_file}" src_time)
            file(TIMESTAMP "${dst_path}" dst_time)
            # copy only if src_time > dst_time
            if(src_time STRGREATER dst_time)
                set(need_copy TRUE)
            endif()
        endif()

        # add to copy list
        if(need_copy)
            list(APPEND copy_src_list "${src_file}")
            list(APPEND copy_dst_list "${dst_dir}")
        endif()
    endforeach()

    # execute copy
    if(copy_src_list)
        message(STATUS "[smart_copy] Start to copy ...")
        foreach(src_file dest_dir IN ZIP_LISTS copy_src_list copy_dst_list) # CMake 3.17+
            file(COPY "${src_file}" DESTINATION "${dest_dir}")
            message(STATUS "    Copy ${src_file} to dir ${dest_dir}/")
        endforeach()
        message(STATUS "[smart_copy] Finish copying files (Updated only)")
    else()
        message(STATUS "[smart_copy] No updated files to copy")
    endif()
endfunction()

## smart copy ${CMAKE_SOURCE_DIR}/${targetdir} to ${CMAKE_BINARY_DIR} with struct
function(smart_copy_dir tar_dir)
    message(STATUS "[smart_copy] copy directory \"${CMAKE_SOURCE_DIR}/${tar_dir}\", keep inner struct.")
    file(GLOB_RECURSE file_list LIST_DIRECTORIES false "${CMAKE_SOURCE_DIR}/${tar_dir}/*")
    smart_copy_files("${file_list}" OFF)
endfunction()


# Execute File
file(GLOB_RECURSE SRC_INNER ${CMAKE_SOURCE_DIR}/src/*.cpp)
# file(GLOB_RECURSE LIB_SRC ${CMAKE_SOURCE_DIR}/lib/*.cpp)
message(STATUS "Source files: ${SRC_INNER}")
# message(STATUS "Lib Source files: ${LIB_SRC}")

add_executable(Nearlighter
    main.cpp
    ${SRC_INNER}
    # ${LIB_SRC}
)


# Include & Link Directory
target_include_directories(Nearlighter PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    # ${CMAKE_SOURCE_DIR}/lib
)


# Assets Copy
smart_copy_dir("assets")


