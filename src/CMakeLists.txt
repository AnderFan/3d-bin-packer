# src/CMakeLists.txt

add_library(packing_solver STATIC
  logic_handler.cpp
  pal_box.cpp
  pal_handle.cpp
  pal_meb.cpp
  pal_zone.cpp
)

add_library(BoxPacking::solver ALIAS packing_solver)

target_include_directories(packing_solver
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include/packing
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(packing_solver PRIVATE
  packing_warnings
)

target_compile_options(packing_solver PRIVATE
    $<$<CONFIG:Release>:
        -O3
        -march=native
    >
)
