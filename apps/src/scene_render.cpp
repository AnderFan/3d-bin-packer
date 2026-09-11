#include "scene_render.hpp"
#include <array>
#include <climits>
#include <vector>

#include "raylib.h"
#include "rcamera.h"
#include "rlgl.h"

void render_box(Pallet &pal) {
  for (auto bx : pal.placed_boxes) {
    const Vector3 center = {bx.pos.x + bx.size.width * 0.5f,
                            bx.pos.y + bx.size.height * 0.5f,
                            bx.pos.z + bx.size.depth * 0.5f};
    DrawCube(center, static_cast<float>(bx.size.width),
             static_cast<float>(bx.size.height),
             static_cast<float>(bx.size.depth), bx.color);
  }
}

void fill_pallet(Pallet &pal) {
  Box bx1;
  bx1.size = {300, 500, 200};
  bx1.pos = {0, 0, 0};
  Color color1 = GRAY;
  bx1.color = color1;

  pal.placed_boxes.push_back(bx1);

  Box bx2;
  bx2.size = {100, 100, 100};
  bx2.pos = {400, 0, 0};
  Color color2 = GREEN;
  bx2.color = color2;
  pal.placed_boxes.push_back(bx2);
}

void test_starter() {
  Pallet pal;
  pal.size = {1200, 1555, 800};
  fill_pallet(pal);
  render_box(pal);
}

void DrawPalletGrid(
    Size size, float step) { // The bottom left edge of the pallet is always
                             // 0.0. The box cannot have a negative position.
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
