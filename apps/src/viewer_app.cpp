#include "viewer_app.hpp"
#include "camera_controller.hpp"
#include "scene_render.hpp"

#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "rlgl.h"

int ViewerApp(const int screenWidth, const int screenHeight) {
  InitWindow(screenWidth, screenHeight, "3dBox");

  // Define the camera to look into our 3d world
  Camera3D camera{};
  camera.position = Vector3{0.0f, 1000.0f, 10.0f}; // Camera position
  camera.target = Vector3{0.0f, 0.0f, 0.0f};       // Camera looking at point
  camera.up =
      Vector3{0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 45.0f;           // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera mode type

  rlSetClipPlanes(10.0, 20000.0);
  SetTargetFPS(60); // Set our game to run at 60 frames-per-second
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
    DrawGrid(1000, 10.0f);

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
