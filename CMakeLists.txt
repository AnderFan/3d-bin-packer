cmake_minimum_required(VERSION 3.20)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(3d-bin-packer
    VERSION 0.1.0
    DESCRIPTION "3D Bin Packing"
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Unit Test" ON)
option(BUILD_BENCHMARKS "Collect performance measurements" OFF)

add_library(packing_warnings INTERFACE)
target_compile_options(packing_warnings INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        $<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>
        $<$<CONFIG:Debug>:-g3 -O0 -fno-omit-frame-pointer>
    >
)
if(ENABLE_ASAN)
    target_compile_options(packing_warnings INTERFACE -fsanitize=address,undefined)
    target_link_options(packing_warnings INTERFACE -fsanitize=address,undefined)
endif()

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
unset(GLFW_USE_WAYLAND CACHE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

add_subdirectory(src)
add_subdirectory(apps)


if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
