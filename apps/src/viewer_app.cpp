#include "viewer_app.hpp"
#include "camera_controller.hpp"
#include "scene_render.hpp"

#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "rlgl.h"

namespace {
constexpr int DefaultFps = 40;
constexpr float Fov = 45.0f;
} // namespace

int ViewerApp(const int screenWidth, const int screenHeight, Size pal_size) {
  InitWindow(screenWidth, screenHeight, "3dBox");

  // Define the camera to look into our 3d world
  Camera3D camera{};
  camera.position = Vector3{1000.0f, 1000.0f, 10.0f}; // Camera position
  float target_x = static_cast<float>(pal_size.width / 2);
  float target_z = static_cast<float>(pal_size.depth / 2);
  camera.target = Vector3{target_x, 0.0f, target_z}; // Camera looking at point
  camera.up =
      Vector3{0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = Fov;             // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera mode type

  rlSetClipPlanes(10.0, 20000.0);
  SetTargetFPS(DefaultFps); // Set our game to run at 60 frames-per-second
  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {

    // Translate based on mouse right click
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      UpdateCamera(&camera, CAMERA_THIRD_PERSON);
    }
    UpdateZoom(camera);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);

    test_starter();
    DrawPalletGrid(pal_size, 10.0f);
    EndMode3D();

    float distance = Vector3Distance(camera.position, camera.target);
    DrawText(TextFormat("distance %06.3f", distance), 10, 100, 20, DARKGRAY);
    DrawText("Welcome to the third dimension!", 10, 40, 20, DARKGRAY);

    DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  CloseWindow();

  return 0;
}
