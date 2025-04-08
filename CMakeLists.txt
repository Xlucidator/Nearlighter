
cmake_minimum_required(VERSION 3.17)
project(Nearlighter)

# Add CMake Modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(smart_copy)


# Build Option
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: Debug Release" FORCE)
endif()
add_compile_definitions("$<$<CONFIG:Debug>:DEBUG>")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")


# C++ Standard
set(CMAKE_CXX_STANDARD 17)


# Execute File
file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
message(STATUS "Source files: ${SRC_FILES}")
add_executable(Nearlighter
    ${SRC_FILES}
)


# Include & Link Directory
target_include_directories(Nearlighter PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/general
)


# Assets Copy
smart_copy_dir("assets")


