# apps/CMakeLists.txt

add_executable(packing_viewer
    src/camera_controller.cpp
    src/viewer_app.cpp
    src/scene_render.cpp
    src/ui_menu.cpp
)

target_link_libraries(packing_viewer PRIVATE
    packing_warnings
    BoxPacking::solver
    raylib
)

target_include_directories(packing_viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/packing
    ${CMAKE_SOURCE_DIR}/include
)
