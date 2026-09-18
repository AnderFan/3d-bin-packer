#include "ui_menu.hpp"
#include "data.hpp"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raylib.h"
#include "types.hpp"
#include <string>

bool DrawMenu(Pallet &pal, std::vector<Box *> &boxes, bool &data_changed) {
  static int tab_scroll = 0;
  static int active_tab = 0;
  GuiTabBar((Rectangle){0, 0, 150, 24}, "Data Input", &tab_scroll, &active_tab);

  // Буферы для добавления ОДНОГО типа коробки
  static int cur_w = 100, cur_h = 100, cur_d = 100;
  static int cur_count = 10, cur_mass = 20;
  static bool cur_full_rotate = false;

  // ID поля, находящегося в режиме редактирования (-1 — ничего не
  // редактируется)
  static int active_box = -1;

  // Лямбда-хелпер для компактной отрисовки и переключения режима ввода
  auto DrawValBox = [&](Rectangle bounds, int *val, int min_val, int max_val,
                        int id) {
    if (GuiValueBox(bounds, NULL, val, min_val, max_val, active_box == id)) {
      active_box = (active_box == id) ? -1 : id;
    }
  };

  // Текстовый буфер для отображения списка в ListView
  static std::string list_text = "";
  static int list_scroll = 0;
  static int list_active = -1;

  // =======================================================
  // 1. ПАРАМЕТРЫ ПАЛЛЕТЫ (пишутся напрямую в поля pal)
  // =======================================================
  GuiLabel((Rectangle){20, 35, 200, 20}, "Pallet parameters (mm):");

  GuiLabel((Rectangle){20, 60, 100, 24}, "Width (X):");
  DrawValBox((Rectangle){130, 60, 120, 24}, &pal.size.width, 100, 5000, 0);

  GuiLabel((Rectangle){20, 90, 100, 24}, "Height (Y):");
  DrawValBox((Rectangle){130, 90, 120, 24}, &pal.size.height, 100, 5000, 1);

  GuiLabel((Rectangle){20, 120, 100, 24}, "Depth (Z):");
  DrawValBox((Rectangle){130, 120, 120, 24}, &pal.size.depth, 100, 5000, 2);

  GuiLabel((Rectangle){20, 150, 100, 24}, "Max weight (g):");
  DrawValBox((Rectangle){130, 150, 120, 24}, &pal.max_mass, 1, 100000, 3);

  // =======================================================
  // 2. ДОБАВЛЕНИЕ КОРОБОК
  // =======================================================
  GuiLabel((Rectangle){20, 190, 200, 20}, "Add boxes:");

  GuiLabel((Rectangle){20, 215, 100, 24}, "Width (mm):");
  DrawValBox((Rectangle){130, 215, 120, 24}, &cur_w, 10, 3000, 4);

  GuiLabel((Rectangle){20, 245, 100, 24}, "Height (mm):");
  DrawValBox((Rectangle){130, 245, 120, 24}, &cur_h, 10, 3000, 5);

  GuiLabel((Rectangle){20, 275, 100, 24}, "Depth (mm):");
  DrawValBox((Rectangle){130, 275, 120, 24}, &cur_d, 10, 3000, 6);

  GuiLabel((Rectangle){20, 305, 100, 24}, "Quantity:");
  DrawValBox((Rectangle){130, 305, 120, 24}, &cur_count, 1, 1000, 7);

  // Флаг неограниченного числа коробок
  GuiCheckBox((Rectangle){260, 309, 16, 16}, "Max.", &pal.hMaxQtyCheck);

  GuiLabel((Rectangle){20, 335, 100, 24}, "Weight (g):");
  DrawValBox((Rectangle){130, 335, 120, 24}, &cur_mass, 1, 50000, 8);

  GuiCheckBox((Rectangle){130, 365, 16, 16}, "Full box rotation?",
              &cur_full_rotate);

  // Добавление коробок сразу в вектор boxes
  if (GuiButton((Rectangle){20, 395, 140, 30}, "Add boxes")) {
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
    if (!list_text.empty())
      list_text += ";";
    list_text += row;
    data_changed = true;
  }

  // Очистка списка коробок
  if (GuiButton((Rectangle){170, 395, 140, 30}, "Clear list")) {
    for (auto *b : boxes)
      delete b;
    boxes.clear();
    list_text.clear();
    list_active = -1;
    data_changed = true;
  }

  // =======================================================
  // 3. СПИСОК КОРОБОК И РЕЖИМ УКЛАДКИ
  // =======================================================
  GuiLabel((Rectangle){340, 35, 200, 20}, "Added box types:");
  GuiListView((Rectangle){340, 60, 480, 260}, list_text.c_str(), &list_scroll,
              &list_active);

  GuiLabel((Rectangle){340, 335, 120, 20}, "Packing method:");

  bool is_vol = (pal.center_mass_or_max_volume == 1);
  bool is_cm = (pal.center_mass_or_max_volume == 0);

  if (GuiCheckBox((Rectangle){340, 365, 16, 16}, "By center of mass", &is_cm)) {
    pal.center_mass_or_max_volume = 0;
  }
  if (GuiCheckBox((Rectangle){480, 365, 16, 16}, "By max volume", &is_vol)) {
    pal.center_mass_or_max_volume = 1;
  }

  if (pal.center_mass_or_max_volume != 1)
    GuiDisable();
  GuiCheckBox((Rectangle){480, 395, 16, 16}, "Forbid incomplete layers",
              &pal.lim_lay);
  if (pal.center_mass_or_max_volume != 1)
    GuiEnable();

  // =======================================================
  // 4. КНОПКА ЗАПУСКА
  // =======================================================
  return GuiButton((Rectangle){20, (float)GetScreenHeight() - 50,
                               (float)GetScreenWidth() - 40, 35},
                   "Calculate packing");
}
