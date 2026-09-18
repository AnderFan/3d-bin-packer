#include "viewer_app.hpp"
#include "camera_controller.hpp"
#include "data.hpp"
#include "pal_handle.hpp"
#include "raylib.h"
#include "rcamera.h"
#include "rlgl.h"
#include "scene_render.hpp"
#include "ui_menu.hpp"
#include <cstddef>
#include <vector>

namespace {
constexpr int DefaultFps = 40;
constexpr float Fov = 45.0f;
constexpr int screenWidth = 1280;
constexpr int screenHeight = 720;
} // namespace

int main() {
  Pallet pal;
  InitWindow(screenWidth, screenHeight, "3dBox");
  SetExitKey(KEY_NULL);
  Camera3D camera{};
  camera.position = Vector3{2000.0f, 2000.0f, 10.0f}; // Camera position
  float target_x = static_cast<float>(pal.size.width / 2);
  float target_y = pal.max_height_box / 2;
  float target_z = static_cast<float>(pal.size.depth / 2);
  camera.target =
      Vector3{target_x, target_y, target_z}; // Camera looking at point
  camera.up =
      Vector3{0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = Fov;             // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera mode type

  rlSetClipPlanes(10.0, 20000.0);
  SetTargetFPS(DefaultFps); // Set our game to run at 60 frames-per-second
  bool in_menu = true;
  Setting setting;
  std::vector<Box *> total_box;
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      UpdateCamera(&camera, CAMERA_THIRD_PERSON);
    }
    UpdateZoom(camera);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    if (in_menu) {
      if (DrawMenu(pal, total_box, setting)) {
        pallet_handle(&pal, total_box, setting);
        camera.target.y = pal.max_height_box / 3;
        in_menu = false;
      }
    } else {
      BeginMode3D(camera);
      if (IsKeyPressed(KEY_ESCAPE)) {
        in_menu = true;
      }
      DrawPalletGrid(pal.size, 10.0f);
      if (!pal.placed_boxes.empty()) {
        for (size_t i = 0; auto box : pal.placed_boxes) {
          auto center = GetPosCenter(box);
          auto color = GetBoxColor(i);
          DrawCube(center, box->size.width, box->size.height, box->size.depth,
                   color);
          i++;
        }
      }

      EndMode3D();
    }
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
