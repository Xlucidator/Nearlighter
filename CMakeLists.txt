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

option(NEARLIGHTER_ENABLE_EXTRA_WARNINGS "Enable additional compiler warnings." OFF)

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

# Platform policy:
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

if(NEARLIGHTER_ENABLE_EXTRA_WARNINGS AND NOT MSVC)
    list(APPEND NEARLIGHTER_DEBUG_FLAGS          -Wextra)
    list(APPEND NEARLIGHTER_RELEASE_FLAGS        -Wextra)
    list(APPEND NEARLIGHTER_RELWITHDEBINFO_FLAGS -Wextra)
endif()

function(nearlighter_apply_build_options target_name)
    target_compile_options(${target_name} PRIVATE
        $<$<CONFIG:Debug>:${NEARLIGHTER_DEBUG_FLAGS}>
        $<$<CONFIG:Release>:${NEARLIGHTER_RELEASE_FLAGS}>
        $<$<CONFIG:RelWithDebInfo>:${NEARLIGHTER_RELWITHDEBINFO_FLAGS}>
    )
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

set(NEARLIGHTER_CORE_SOURCES
    src/bvh/aabb.cpp
    src/bvh/bvhnode.cpp
    src/camera/camera.cpp
    src/general/color.cpp
    src/general/interval.cpp
    src/medium/constant_medium.cpp
    src/scene.cpp
    src/shape/quad.cpp
    src/shape/sphere.cpp
    src/utils/imageloader.cpp
    src/utils/perlin.cpp
)

add_library(nearlighter_core STATIC
    ${NEARLIGHTER_CORE_SOURCES}
)

target_include_directories(nearlighter_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/general"
)

target_link_libraries(nearlighter_core PUBLIC
    glm::glm
    stb
)

nearlighter_apply_build_options(nearlighter_core)
message(STATUS "Target [nearlighter_core] configured.")


# =============================================================
# 4. Nearlighter Executable
# =============================================================

add_executable(Nearlighter
    src/main.cpp
)

target_link_libraries(Nearlighter PRIVATE
    nearlighter_core
)

nearlighter_apply_build_options(Nearlighter)
message(STATUS "Target [Nearlighter] configured.")


# =============================================================
# 5. Runtime Assets
# =============================================================

smart_copy_dir("assets")
