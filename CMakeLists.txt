cmake_minimum_required( VERSION 3.5 )

project( my-3D-viewer )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set( GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only" )
set( BUILD_SHARED_LIBS OFF CACHE BOOL "")
set( ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set( ASSIMP_INSTALL OFF CACHE BOOL "")
set( ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "")
set( ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "")

include_directories(3rdparty/glad/include)
include_directories(3rdparty/stb_image)
include_directories(3rdparty/cxxopts)
include_directories(3rdparty/LearnOpenGL)
include_directories(3rdparty)
add_subdirectory( "3rdparty/glfw-3.4" )
add_subdirectory( "3rdparty/assimp-6.0.2" )

add_executable(texture
        src/main.cpp 
        3rdparty/stb_image/stb.cpp
        3rdparty/glad/src/glad.c
        src/ColmapViews.cpp
        src/buffer.cpp
        src/CLIargs.cpp)
target_link_libraries( texture glfw assimp)


add_executable(render
        src/test.cpp 
        3rdparty/stb_image/stb.cpp
        3rdparty/glad/src/glad.c
        src/ColmapViews.cpp
        src/buffer.cpp)
target_link_libraries(render glfw assimp)
