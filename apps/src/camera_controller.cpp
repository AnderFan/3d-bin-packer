#include "camera_controller.hpp"

#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"

void UpdateZoom(Camera3D &camera) {
  float wheel = GetMouseWheelMove();
  if (wheel != 0.0f) {
    float distance = Vector3Distance(camera.position, camera.target);
    if (distance > 10.0f)
      CameraMoveToTarget(&camera, -wheel * 10);
    if (distance < 10.0f && wheel < 0.0f)
      CameraMoveToTarget(&camera, -wheel * 10);
  }
}
