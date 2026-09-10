# apps/CMakeLists.txt

include(FetchContent)

# 1. Отключаем лишнее в Raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

# 2. Настройка под современный Linux (поддержка Wayland и X11)
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
unset(GLFW_USE_WAYLAND CACHE)

# 3. Декларация Raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

# 4. Исполняемый файл
add_executable(packing_viewer
    src/main.cpp
    src/camera_controller.cpp
    src/viewer_app.cpp
    src/scene_render.cpp
)

target_link_libraries(packing_viewer PRIVATE
    packing_warnings
    raylib
)

target_include_directories(packing_viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
