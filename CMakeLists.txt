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
    >
)

add_subdirectory(src)
add_subdirectory(apps)


if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
