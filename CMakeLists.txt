
cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)

project(MyPathTracer)

include_directories(lib)
include_directories(src)
include_directories(src/shape)

set( SRC 
    main.cpp
    src/color.cpp
    src/camera.cpp
    src/shape/sphere.cpp
)
# file(GLOB SRC_FILES "src/*.cpp")

add_executable(main ${SRC})