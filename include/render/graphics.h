#pragma once
#ifndef GRAPHICS_H_INCLUDED
#define GRAPHICS_H_INCLUDED

#include "data.h"

// Структура для управления камерой
struct Camera {
  float distance = PALLET_X * 2;
  float angleX = 30.0f;
  float angleY = 45.0f;
  float targetX = PALLET_X / 2.0f;
  float targetY = PALLET_Y / 2.0f;
  float targetZ = PALLET_Z / 2.0f;
};

// Структура для GUI состояния
struct GUIState {
  bool show_input_window = true;
  bool show_visualization = false;
  bool calculation_done = false;
  bool show_stats = true;

  // Параметры паллеты
  int pallet_width = PALLET_X;
  int pallet_height = PALLET_Y;
  int pallet_depth = PALLET_Z;
  int pallet_max_mass = 1050; // Максимальный вес паллеты (кг)

  bool use_center_mass = false;
  bool use_max_volume = false;

  bool hMaxQtyCheck = false; // Флаг для ограничения количества коробок
  bool hLimLayCheck = false; // Новый флаг из чекбокса "Параметры размещения"

  // Список добавленных коробок
  vector<box_property> box_types;

  // Указатель на текущий паллет
  vector<pallet *> current_pallet = {nullptr, nullptr};
};

// Основные функции графики
bool init_graphics(int width, int height, const char *title);
void cleanup_graphics();
bool should_close_window();
void process_input();

// Функции рендеринга
void begin_frame();
void end_frame();
void render_3d_scene(pallet *pal_ptr, Camera *cam);
void render_gui(GUIState *state);

// Утилиты рендеринга
void draw_box_3d(box *box_ptr, int index);
void draw_pallet_base();
void draw_grid();
void draw_axes();
void setup_camera(Camera *cam);

// GUI функции
void render_input_panel(GUIState *state);
void render_stats_panel(pallet *pal_ptr);
void render_box_list(GUIState *state);
void update_tabs();

// Основной цикл графического режима
void run_graphics_mode();

#endif
