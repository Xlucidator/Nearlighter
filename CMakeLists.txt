cmake_minimum_required(VERSION 3.17)

project(Nearlighter
    VERSION 0.1.0
    LANGUAGES CXX
)


# =============================================================
# 1. Basic Settings
# =============================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type." FORCE)
endif()

# Option settings
option(ENABLE_EXTRA_WARNINGS "Enable additional compiler warnings." OFF)

# Import custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(smart_copy)

# Enable LTO/IPO for Release builds when the toolchain supports it.
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    message(STATUS "LTO/IPO is enabled for Release build.")
else()
    message(WARNING "LTO/IPO is not supported: ${IPO_ERROR}")
endif()

# Compile options for different platform
if(UNIX AND NOT MINGW)
    # 1. Unix-like is the primary development target.
    message(STATUS "Building on Unix-like platform.")
    set(NEARLIGHTER_DEBUG_FLAGS           -O0 -Wall)
    set(NEARLIGHTER_RELEASE_FLAGS         -march=native -funroll-loops)
    set(NEARLIGHTER_RELWITHDEBINFO_FLAGS  -march=native)
elseif(MINGW)
    # 2. MinGW is the preferred Windows toolchain when possible.
    message(STATUS "Building on Windows MinGW.")
    set(NEARLIGHTER_DEBUG_FLAGS           -O0 -g -Wall)
    set(NEARLIGHTER_RELEASE_FLAGS         -march=native -funroll-loops -static)
    set(NEARLIGHTER_RELWITHDEBINFO_FLAGS  -march=native -static)
elseif(MSVC)
    # 3. MSVC is supported, but not the primary tuning target.
    message(STATUS "Building on Windows MSVC.")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set(NEARLIGHTER_DEBUG_FLAGS           /Od /Zi /W4 /permissive-)
    set(NEARLIGHTER_RELEASE_FLAGS         /Oi /Ot /GL)
    set(NEARLIGHTER_RELWITHDEBINFO_FLAGS  /Oi /Ot)
else()
    message(STATUS "Building on an unknown platform; using conservative flags.")
    set(NEARLIGHTER_DEBUG_FLAGS           -O0 -Wall)
    set(NEARLIGHTER_RELEASE_FLAGS)
    set(NEARLIGHTER_RELWITHDEBINFO_FLAGS)
endif()

if(ENABLE_EXTRA_WARNINGS AND NOT MSVC)
    list(APPEND NEARLIGHTER_DEBUG_FLAGS          -Wextra)
    list(APPEND NEARLIGHTER_RELEASE_FLAGS        -Wextra)
    list(APPEND NEARLIGHTER_RELWITHDEBINFO_FLAGS -Wextra)
endif()

function(target_apply_build_options target_name)
    # Compiler flags for different build types
    target_compile_options(${target_name} PRIVATE
        $<$<CONFIG:Debug>:${NEARLIGHTER_DEBUG_FLAGS}>
        $<$<CONFIG:Release>:${NEARLIGHTER_RELEASE_FLAGS}>
        $<$<CONFIG:RelWithDebInfo>:${NEARLIGHTER_RELWITHDEBINFO_FLAGS}>
    )
    # Custom definitions for different build types
    target_compile_definitions(${target_name} PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
    )
endfunction()


# =============================================================
# 2. Third-party Dependencies
# =============================================================

# --- GLM (version: 1.1.0, commit: 6f14f4792a0c, describe: 1.0.0-139-g6f14f479) ---
add_subdirectory(thirdparty/glm)

# --- stb (commit: 31c1ad374564, branch snapshot: master) ---
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
)


# =============================================================
# 3. Nearlighter Core
# =============================================================

set(CORE_SOURCES
    src/nearlighter/accel/bvh_node.cpp
    src/nearlighter/base/color.cpp
    src/nearlighter/base/interval.cpp
    src/nearlighter/geometry/aabb.cpp
    src/nearlighter/geometry/quad.cpp
    src/nearlighter/geometry/shape_list.cpp
    src/nearlighter/geometry/sphere.cpp
    src/nearlighter/io/image_loader.cpp
    src/nearlighter/material/dielectric.cpp
    src/nearlighter/material/diffuse_light.cpp
    src/nearlighter/material/isotropic.cpp
    src/nearlighter/material/lambertian.cpp
    src/nearlighter/material/material.cpp
    src/nearlighter/material/metal.cpp
    src/nearlighter/medium/constant_medium.cpp
    src/nearlighter/render/camera.cpp
    src/nearlighter/scene/scene.cpp
    src/nearlighter/texture/checker_texture.cpp
    src/nearlighter/texture/image_texture.cpp
    src/nearlighter/texture/noise_texture.cpp
    src/nearlighter/texture/perlin.cpp
    src/nearlighter/texture/solid_texture.cpp
)


# =============================================================
# 4. Nearlighter Application Build
# =============================================================

# --- CLI Version ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(Nearlighter
        src/main.cpp
        ${CORE_SOURCES}
    )

    target_include_directories(Nearlighter PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    target_link_libraries(Nearlighter PRIVATE
        glm::glm
        stb
    )

    target_apply_build_options(Nearlighter)
    message(STATUS "Target [Nearlighter] configured.")
endif()


# =============================================================
# 5. Runtime Assets
# =============================================================

smart_copy_dir("assets")
