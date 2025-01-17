
cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)

project(MyPathTracer)

# Pre-define
add_definitions(-DDEBUG)


# Include & Link Directory
include_directories(
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/src
)


# Debug
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g")


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