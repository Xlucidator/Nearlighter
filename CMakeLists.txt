
cmake_minimum_required(VERSION 3.12)

project(MyPathTracer)

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

# Include & Link Directory
include_directories(
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/src
)

# Execute File
file(GLOB_RECURSE SRC_INNER ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE LIB_SRC ${CMAKE_SOURCE_DIR}/lib/*.cpp)
message(STATUS "Source files: ${SRC_INNER}")
message(STATUS "Lib Source files: ${LIB_SRC}")

add_executable(MyPathTracer
    main.cpp
    ${SRC_INNER}
    ${LIB_SRC}
)