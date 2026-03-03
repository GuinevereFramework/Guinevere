
cmake_minimum_required(VERSION 3.20)

project(Guinevere LANGUAGES CXX)

option(GUINEVERE_BUILD_TESTS "Build Guinevere tests" ON)
option(GUINEVERE_BUILD_EXAMPLES "Build Guinevere examples" ON)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3
)
FetchContent_MakeAvailable(freetype)

find_package(OpenGL REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(guinevere_warnings INTERFACE)
if(MSVC)
    target_compile_options(guinevere_warnings INTERFACE /W4 /WX /permissive- /utf-8)
else()
    target_compile_options(guinevere_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_subdirectory(src)

if(GUINEVERE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(GUINEVERE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
