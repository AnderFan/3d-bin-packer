#include "scene_render.hpp"

#include "data.hpp"
#include "raylib.h"
#include "rcamera.h"
#include "rlgl.h"
#include <cmath>

void DrawPalletGrid(Size size, float step) {
  rlBegin(RL_LINES);
  rlColor4ub(0, 0, 0, 255);

  for (float x = 0.0f; x <= size.width; x += step) {
    rlVertex3f(0.0f + x, 0.0f, 0.0f);
    rlVertex3f(0.0f + x, 0.0f, 0.0f + size.depth);
  }
  rlVertex3f(0.0f + size.width, 0.0f, 0.0f);
  rlVertex3f(0.0f + size.width, 0.0f, 0.0f + size.depth);

  for (float z = 0.0f; z <= size.depth; z += step) {
    rlVertex3f(0.0f, 0.0f, 0.0f + z);
    rlVertex3f(0.0f + size.width, 0, 0.0f + z);
  }
  rlVertex3f(0.0f, 0.0f, 0.0f + size.depth);
  rlVertex3f(0.0f + size.width, 0, 0.0f + size.depth);

  rlEnd();
}
::Color GetBoxColor(int id) {
  float hue = std::fmod(static_cast<float>(id) * 137.508f, 360.0f);

  return ColorFromHSV(hue, 0.75f, 0.9f);
}

::Vector3 GetPosCenter(Box *box) {
  ::Vector3 center = {static_cast<float>(box->pos.x) +
                          static_cast<float>(box->size.width) * 0.5f,
                      static_cast<float>(box->pos.y) +
                          static_cast<float>(box->size.height) * 0.5f,
                      static_cast<float>(box->pos.z) +
                          static_cast<float>(box->size.depth) * 0.5f};
  return center;
}
