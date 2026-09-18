#include "ui_menu.hpp"
#include "data.hpp"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raylib.h"
#include "types.hpp"
#include <iostream>
#include <string>

bool DrawMenu(Pallet &pal, std::vector<Box *> &boxes, Setting &setting) {
  GuiSetStyle(DEFAULT, TEXT_SIZE, 24);

  static int tab_scroll = 0;
  static int active_tab = 0;
  GuiTabBar((Rectangle){0, 0, 225, 36}, "Data Input", &tab_scroll, &active_tab);

  static int cur_w = 100, cur_h = 100, cur_d = 100;
  static int cur_count = 10, cur_mass = 20;
  static bool cur_full_rotate = false;

  static int active_box = -1;

  auto DrawValBox = [&](Rectangle bounds, int *val, int min_val, int max_val,
                        int id) {
    if (GuiValueBox(bounds, NULL, val, min_val, max_val, active_box == id)) {
      active_box = (active_box == id) ? -1 : id;
    }
  };

  static std::string list_text = "";
  static int list_scroll = 0;
  static int list_active = -1;

  GuiLabel((Rectangle){30, 52.5f, 300, 30}, "Pallet parameters (mm):");

  GuiLabel((Rectangle){30, 90, 150, 36}, "Width (X):");
  DrawValBox((Rectangle){195, 90, 180, 36}, &pal.size.width, 100, 5000, 0);

  GuiLabel((Rectangle){30, 135, 150, 36}, "Height (Y):");
  DrawValBox((Rectangle){195, 135, 180, 36}, &pal.size.height, 100, 5000, 1);

  GuiLabel((Rectangle){30, 180, 150, 36}, "Depth (Z):");
  DrawValBox((Rectangle){195, 180, 180, 36}, &pal.size.depth, 100, 5000, 2);

  GuiLabel((Rectangle){30, 225, 150, 36}, "Carrying (g):");
  DrawValBox((Rectangle){195, 225, 180, 36}, &pal.max_mass, 1, 100000, 3);

  GuiLabel((Rectangle){30, 285, 300, 30}, "Add boxes:");

  GuiLabel((Rectangle){30, 322.5f, 150, 36}, "Width (mm):");
  DrawValBox((Rectangle){195, 322.5f, 180, 36}, &cur_w, 10, 3000, 4);

  GuiLabel((Rectangle){30, 367.5f, 150, 36}, "Height (mm):");
  DrawValBox((Rectangle){195, 367.5f, 180, 36}, &cur_h, 10, 3000, 5);

  GuiLabel((Rectangle){30, 412.5f, 150, 36}, "Depth (mm):");
  DrawValBox((Rectangle){195, 412.5f, 180, 36}, &cur_d, 10, 3000, 6);

  GuiLabel((Rectangle){30, 457.5f, 150, 36}, "Quantity:");
  DrawValBox((Rectangle){195, 457.5f, 180, 36}, &cur_count, 1, 1000, 7);

  if (boxes.size() >= 2)
    GuiDisable();
  GuiCheckBox((Rectangle){390, 463.5f, 24, 24}, "Max.", &setting.hMaxQtyCheck);
  GuiEnable();
  GuiLabel((Rectangle){30, 502.5f, 150, 36}, "Weight (g):");
  DrawValBox((Rectangle){195, 502.5f, 180, 36}, &cur_mass, 1, 50000, 8);

  GuiCheckBox((Rectangle){195, 547.5f, 24, 24}, "Full box rotation?",
              &cur_full_rotate);

  if (setting.hMaxQtyCheck && boxes.size() >= 1)
    GuiDisable();
  if (GuiButton((Rectangle){30, 592.5f, 210, 45}, "Add boxes")) {
    for (int i = 0; i < cur_count; ++i) {
      Box *b = new Box();
      b->size = Size{cur_w, cur_h, cur_d};
      b->mass = cur_mass;
      b->full_rotateble = cur_full_rotate;
      b->placed = false;
      b->pos = {-1, -1, -1};
      boxes.push_back(b);
    }

    std::string row = TextFormat("%dx%dx%d mm | %d pcs | %d g", cur_w, cur_h,
                                 cur_d, cur_count, cur_mass);
    if (cur_full_rotate)
      row += "| Full rotateble";

    if (!list_text.empty())
      list_text += ";";
    std::cout << "DEBUG cur_full_rotate = " << cur_full_rotate << std::endl;
    list_text += row;
  }
  GuiEnable();
  if (GuiButton((Rectangle){255, 592.5f, 210, 45}, "Clear list")) {
    for (auto *b : boxes)
      delete b;
    boxes.clear();
    list_text.clear();
    list_active = -1;
  }

  GuiLabel((Rectangle){510, 52.5f, 300, 30}, "Added box types:");
  GuiListView((Rectangle){510, 90, 720, 390}, list_text.c_str(), &list_scroll,
              &list_active);

  GuiLabel((Rectangle){510, 502.5f, 180, 30}, "Packing method:");

  bool is_vol = (setting.center_mass_or_max_volume == 1);
  bool is_cm = (setting.center_mass_or_max_volume == 0);

  if (GuiCheckBox((Rectangle){510, 547.5f, 24, 24}, "By center of mass",
                  &is_cm)) {
    setting.center_mass_or_max_volume = 0;
  }
  if (GuiCheckBox((Rectangle){820, 547.5f, 24, 24}, "By max volume", &is_vol)) {
    setting.center_mass_or_max_volume = 1;
  }

  if (!setting.hMaxQtyCheck)
    GuiDisable();
  GuiCheckBox((Rectangle){820, 592.5f, 24, 24}, "Forbid incomplete layers",
              &setting.lim_lay);

  if (!setting.hMaxQtyCheck)
    GuiEnable();

  return GuiButton((Rectangle){30, (float)GetScreenHeight() - 75.0f,
                               (float)GetScreenWidth() - 60.0f, 52.5f},
                   "Calculate packing");
}
