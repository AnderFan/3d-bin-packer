#include "pal_box.hpp"
#include "data.hpp"
#include "pal_meb.hpp"
#include "types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <omp.h>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#ifdef DEBUG_PALLET
#define DEBUG_LOG(x) cout << x
#define DEBUG_LOG_ENDL(x) cout << x << endl
#else
#define DEBUG_LOG(x)
#define DEBUG_LOG_ENDL(x)
#endif

void height_map_init(Pallet *pal_ptr) {
  int W = pal_ptr->size.width;
  int D = pal_ptr->size.depth;
  pal_ptr->height_map.assign(
      W, std::vector<int>(D, 0)); // инициализируем карту высот нулями
}
int get_max_remaining_box_height(std::vector<Box *> &total_boxes) {
  int max_h = 0;
  for (auto *b : total_boxes) {
    if (b->placed)
      continue;
    // обе ориентации, на всякий случай
    max_h = std::max(max_h, b->size.height);
    // max_h = std::max(max_h, b->size.depth);
  }
  return max_h;
}
CenterMassResult simulate_center_mass(const Pallet *pal_ptr, int box_mass,
                                      double cx_box, double cy_box,
                                      double cz_box) {
  CenterMassResult res{};

  int prev_mass = pal_ptr->total_mass;
  int new_mass = prev_mass + box_mass;
  res.total_mass = new_mass;

  double new_cx, new_cy, new_cz;

  if (prev_mass == 0) {
    // Если на паллете ничего не было – центр масс = центр этой коробки
    new_cx = cx_box;
    new_cy = cy_box;
    new_cz = cz_box;
  } else {
    new_cx =
        (pal_ptr->xyz_mass_centre[0] * (double)prev_mass + cx_box * box_mass) /
        (double)new_mass;
    new_cy =
        (pal_ptr->xyz_mass_centre[1] * (double)prev_mass + cy_box * box_mass) /
        (double)new_mass;
    new_cz =
        (pal_ptr->xyz_mass_centre[2] * (double)prev_mass + cz_box * box_mass) /
        (double)new_mass;
  }

  res.cx = new_cx;
  res.cy = new_cy;
  res.cz = new_cz;

  return res;
}

void center_mass_calculate(Pallet *pal_ptr, Box *box_ptr) {

  double bx = box_ptr->pos.x;
  double by = box_ptr->pos.y;
  double bz = box_ptr->pos.z;

  double bw = box_ptr->size.width;
  double bh = box_ptr->size.height;
  double bd = box_ptr->size.depth;

  // геометрический центр коробки
  double cx_box = bx + bw / 2.0;
  double cy_box = by + bh / 2.0;
  double cz_box = bz + bd / 2.0;

  CenterMassResult cm =
      simulate_center_mass(pal_ptr, box_ptr->mass, cx_box, cy_box, cz_box);

  pal_ptr->total_mass = cm.total_mass;
  pal_ptr->xyz_mass_centre[0] = cm.cx;
  pal_ptr->xyz_mass_centre[1] = cm.cy;
  pal_ptr->xyz_mass_centre[2] = cm.cz;

  DEBUG_LOG_ENDL("Новый центр масс паллета: ("
                 << pal_ptr->xyz_mass_centre[0] << ", "
                 << pal_ptr->xyz_mass_centre[1] << ", "
                 << pal_ptr->xyz_mass_centre[2] << ") с массой "
                 << pal_ptr->total_mass);
}

std::array<int, SCORES_NUM>
access_box_in_zone(Zone *zone_ptr, Box *box, Pallet *pal_ptr, int index,
                   std::vector<Box *> &total_boxes) {

  int com_y_score = INT_MAX;
  int com_center_score = INT_MAX;
  int box_center_score = INT_MAX;
  if (pal_ptr->center_mass_or_max_volume == 0) { // Если укладка по центру масс
    double cx_box = box->temp_pos.x + box->size.width / 2.0;
    double cy_box = box->temp_pos.y + box->size.height / 2.0;
    double cz_box = box->temp_pos.z + box->size.depth / 2.0;

    CenterMassResult cm =
        simulate_center_mass(pal_ptr, box->mass, cx_box, cy_box, cz_box);
    // идеальный центр масс паллета

    // отклонение от центра по XZ
    double dx = cm.cx - pal_ptr->ideal_cx;
    // double dy = cm.cy - pal_ptr->ideal_cy;
    double dz = cm.cz - pal_ptr->ideal_cz;
    double dist2_xz = dx * dx + dz * dz;

    double bdx = cx_box - pal_ptr->ideal_cx;
    double bdz = cz_box - pal_ptr->ideal_cz;
    double bdist2_xz = bdx * bdx + bdz * bdz;

    com_y_score = (int)std::round(cm.cy * 100.0); // ниже центр масс по Y
    com_center_score =
        (int)std::round(dist2_xz * 100.0); // ближе к центру по XZ
    box_center_score = (int)std::round(bdist2_xz * 100.0);
  }

  int w_zone = zone_ptr->size.width - (box->temp_pos.x - zone_ptr->pos.x);
  int y_zone = zone_ptr->size.height - (box->temp_pos.y - zone_ptr->pos.y);
  int d_zone = zone_ptr->size.depth - (box->temp_pos.z - zone_ptr->pos.z);

  int height_diff = zone_ptr->pos.y + box->temp_pos.y;
  if (pal_ptr->center_mass_or_max_volume ==
      1) { // Если укладка по максимальному объему
    if ((pal_ptr->size.width / box->size.width) *
            (pal_ptr->size.depth / box->size.depth) <
        (pal_ptr->size.width / box->size.depth) *
            (pal_ptr->size.depth / box->size.width)) {
      height_diff +=
          500; // если коробка лучше укладывается в другую ориентацию, штрафуем
    }

    if (!pal_ptr->placed_boxes.empty()) {
      auto prev_box = pal_ptr->placed_boxes[pal_ptr->placed_boxes.size() - 1];
      // if (prev_box->size == box->size) {
      //  if (prev_box->rotate != rotate) {
      //    height_diff += 500; // если предыдущая коробка была такого же
      //    размера,
      //                        // но в другой ориентации, штрафуем
      //  }
      //}
    }
  }

  int waste = w_zone * d_zone - box->size.width * box->size.depth;
  int long_side =
      (std::max)(w_zone - box->size.width, d_zone - box->size.depth);
  int short_side =
      (std::min)(w_zone - box->size.width, d_zone - box->size.depth);

  int max_remaining_box_height = get_max_remaining_box_height(total_boxes);
  // if (max_remaining_box_height > bh && (pal_ptr->xyz_size[1] - (by + bh)) <
  // max_remaining_box_height) {
  //	// если после этой укладки не останется места для самой высокой
  // оставшейся коробки 	height_diff += 10000; // штрафуем сильно
  // }
  if (box->ratio < 0.8)
    height_diff += 500; // если ratio меньше 0.8, штрафуем
  if (box->size.height > box->size.width && box->size.height > box->size.depth)
    height_diff += 200;        // преиущественно коробки должны ложиться плашмя
  height_diff += (index * 10); // Коробки отсортированы по убыванию. Чем больше
                               // индекс у коробки тем она меньше

  if (pal_ptr->center_mass_or_max_volume == 0) { // Если укладка по центру масс
    return std::array<int, SCORES_NUM>{
        com_center_score, com_y_score, box_center_score,
        height_diff,      waste,       long_side};
  } else if (pal_ptr->center_mass_or_max_volume ==
             1) { // Если укладка по максимальному объему
    return std::array<int, SCORES_NUM>{height_diff, waste,   long_side,
                                       short_side,  INT_MAX, INT_MAX};
  }
}

bool can_place_box_height(Pallet *pal_ptr, Zone *zone, Size *box_size,
                          Packing::Vector3 &temp_pos, double &ratio) {
  int y0 = zone->pos.y;
  if (y0 == 0) {
    temp_pos = zone->pos;
    ratio = 1.0;
    return true;
  }

  int x0 = zone->pos.x;
  int z0 = zone->pos.z;
  int wz = zone->size.width;
  int dz = zone->size.depth;
  int palW = pal_ptr->size.width;
  int palD = pal_ptr->size.depth;

  if (x0 + wz > palW || z0 + dz > palD || wz < box_size->width ||
      dz < box_size->depth) {
    return false;
  }

  const auto &height_map = pal_ptr->height_map;
  int total_area = box_size->width * box_size->depth;
  int min_support_pixels = (pal_ptr->min_support * total_area);

  int max_start_x = std::min(x0 + 3, x0 + wz - box_size->width);
  int max_start_z = std::min(z0 + 3, z0 + dz - box_size->depth);

  for (int px = x0; px <= max_start_x; ++px) {
    for (int pz = z0; pz <= max_start_z; ++pz) {

      int px_end = px + box_size->width;
      int pz_end = pz + box_size->depth;

      if (px_end > palW || pz_end > palD) {
        continue;
      }

      int supported_area = 0;

      for (int x = px; x < px_end; ++x) {
        const std::vector<int> &row = height_map[x];

        for (int z = pz; z < pz_end; ++z) {
          if (row[z] == y0) {
            ++supported_area;
          }
        }
      }

      if (supported_area >= min_support_pixels) {
        temp_pos = {px, zone->pos.y, pz};
        ratio = static_cast<double>(supported_area) / total_area;
        return true;
      }
    }
  }

  return false;
}
bool is_placement_possible(Pallet *pallet_ptr,
                           std::vector<Box *> &total_boxes) {
  DEBUG_LOG_ENDL("\n=== ПРОВЕРКА ВОЗМОЖНОСТИ РАЗМЕЩЕНИЯ ===");
  DEBUG_LOG_ENDL("Коробок в total_boxes: " << total_boxes.size());
  DEBUG_LOG_ENDL("Зон доступно: " << pallet_ptr->zone_vector.size());

  bool flag_zone = false, flag_box = false;
  if (pallet_ptr->zone_vector.size() > 0) {
    DEBUG_LOG_ENDL(">>> Есть доступные зоны для размещения коробок");
    flag_zone = true;
  } // если есть хоть одна живая зона

  if (pallet_ptr->hMaxQtyCheck ==
      false) { // если кол-во коробок ограничено, то проверяем есть ли
               // неразмещённые коробки
    for (int i = 0; i < total_boxes.size(); i++) {
      DEBUG_LOG_ENDL("  Box[" << i
                              << "] placed=" << (bool)total_boxes[i]->placed);
      if (total_boxes[i]->placed == false) {
        DEBUG_LOG_ENDL(">>> Найдена неразмещённая коробка, продолжаем");
        flag_box = true;
      }
    } // если есть хоть одна коробка которая не размещена
  } else {
    flag_box = true;
  }

  if (flag_zone && flag_box)
    return true;
  return false;
}

// void split_zone() {
//	return;
// }

void celebrate() { DEBUG_LOG_ENDL("КОРОБКА УЛОЖЕНА!"); }

Box *select_best_box(std::vector<Box *> &total_boxes) {
  Box *best_box_ptr = nullptr;
  std::array<int, SCORES_NUM> best = {
      INT_MAX, INT_MAX, INT_MAX,
      INT_MAX, INT_MAX, INT_MAX};                // тут храним лучшую коробку
  for (int i = 0; i < total_boxes.size(); i++) { // проходимся по коробкам
    if (total_boxes.at(i)->scores < best) {
      best = total_boxes.at(i)->scores;
      best_box_ptr = total_boxes.at(i);
    }
  }
  if (best_box_ptr == nullptr ||
      best == std::array<int, SCORES_NUM>{
                  INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX,
                  INT_MAX}) { // если не нашли ни одной коробки
    DEBUG_LOG_ENDL("Не удалось найти подходящую коробку для размещенияEEE");
    return nullptr;
  }

  DEBUG_LOG_ENDL("Лучшая коробка получила оценки: " << best[0] << " " << best[1]
                                                    << " " << best[2] << " "
                                                    << best[3]);
  return best_box_ptr;
}

void place_box(Pallet *pal_ptr, Zone *zone_ptr, Box *box_ptr,
               std::vector<Box *> &total_boxes) {
  DEBUG_LOG_ENDL("\n--- РАЗМЕЩЕНИЕ КОРОБКИ ---");
  DEBUG_LOG_ENDL("Позиция: (" << box_ptr->temp_xz[0] << ", " << zone_ptr->xyz[1]
                              << ", " << box_ptr->temp_xz[1] << ")");

  box_ptr->pos = box_ptr->temp_pos;

  pal_ptr->placed_boxes.push_back(box_ptr);

  if (pal_ptr->max_height_box == INT_MAX) {
    pal_ptr->max_height_box = pal_ptr->placed_boxes[0]->size.height;
  }
  pal_ptr->max_height_box =
      std::max(pal_ptr->max_height_box, box_ptr->pos.y + box_ptr->size.height);

  DEBUG_LOG_ENDL("Удаляем коробку из total_boxes (было: " << total_boxes.size()
                                                          << ")");
  total_boxes.erase(remove(total_boxes.begin(), total_boxes.end(), box_ptr),
                    total_boxes.end());
  DEBUG_LOG_ENDL("После удаления: " << total_boxes.size());

  box_ptr->placed = true;
  pal_ptr->placed_since_meb++;
  pal_ptr->was_defrag = false;

  int W = pal_ptr->size.width;
  int D = pal_ptr->size.depth;

  if ((int)pal_ptr->height_map.size() != W) {
    std::cerr << "HEIGHT_MAP BAD SIZE: height_map.size()="
              << pal_ptr->height_map.size() << " expected W=" << W << "\n";
  }
  for (int x = 0; x < W; ++x) {
    if ((int)pal_ptr->height_map[x].size() != D) {
      std::cerr << "HEIGHT_MAP BAD SIZE: height_map[" << x
                << "].size()=" << pal_ptr->height_map[x].size()
                << " expected D=" << D << "\n";
    }
  }

  int x_start = box_ptr->pos.x;
  int x_end = x_start + box_ptr->size.width;
  int z_start = box_ptr->pos.z;
  int z_end = z_start + box_ptr->size.depth;
  int new_height = box_ptr->pos.y + box_ptr->size.height;

  DEBUG_LOG_ENDL("Обновляем height_map: x=["
                 << x_start << ".." << x_end << "), z=[" << z_start << ".."
                 << z_end << "), new_height=" << new_height);

  for (int x = x_start; x < x_end; ++x) {
    for (int z = z_start; z < z_end; ++z) {
      pal_ptr->height_map[x][z] =
          std::max(pal_ptr->height_map[x][z], new_height);
    }
  }

  center_mass_calculate(pal_ptr, box_ptr);
}
std::optional<std::vector<Size>> get_correct_full_rotate(Size box, Size zone) {
  if (box.height > zone.height) {
    return std::nullopt;
  }
  const std::array<Size, 2> candidates = {
      Size{box.width, box.height, box.depth},
      Size{box.depth, box.height, box.width}};

  std::vector<Size> correct_size;
  for (const auto &c : candidates) {
    if (c.width <= zone.width && c.depth <= zone.depth) {
      correct_size.push_back(c);
    }
  }
  return correct_size;
}

std::optional<std::vector<Size>> get_correct_rotate(Size box, Size zone) {
  if (box.height > zone.height) {
    return std::nullopt;
  }

  const std::array<Size, 2> candidates = {
      {{box.width, box.height, box.depth}, {box.width, box.height, box.depth}}};

  std::vector<Size> correct_size;

  for (const auto &candidate : candidates) {
    if (candidate.width <= zone.width && candidate.depth <= zone.depth) {
      correct_size.push_back(candidate);
    }
  }
  return correct_size;
}

struct LocalBest {
  std::array<int, SCORES_NUM> score = {INT_MAX, INT_MAX, INT_MAX,
                                       INT_MAX, INT_MAX, INT_MAX};
  Size size;
  Packing::Vector3 pos;
  bool local_any_fit = false;
};
Box *box_placement_handle(Pallet *pal_ptr, Zone *zone_ptr,
                          std::vector<Box *> &total_boxes) {
  DEBUG_LOG_ENDL("\n========== BOX_PLACEMENT_HANDLE ==========");
  DEBUG_LOG_ENDL("Целевая зона: pos=("
                 << zone_ptr->xyz[0] << "," << zone_ptr->xyz[1] << ","
                 << zone_ptr->xyz[2] << ") size=(" << zone_ptr->xyz_size[0]
                 << "x" << zone_ptr->xyz_size[1] << "x" << zone_ptr->xyz_size[2]
                 << ")");
  DEBUG_LOG_ENDL("Коробок для проверки: " << total_boxes.size());
  DEBUG_LOG_ENDL("Текущая масса паллета: " << pal_ptr->total_mass << " / "
                                           << pal_ptr->max_mass);

  Box *cur_box_ptr = nullptr;
  int best_idx = -1;
  std::array<int, SCORES_NUM> best = {INT_MAX, INT_MAX, INT_MAX,
                                      INT_MAX, INT_MAX, INT_MAX};

  int debug_skipped_placed = 0;
  int debug_skipped_mass = 0;
  int debug_skipped_size = 0;
  int debug_skipped_height = 0;
  int debug_skipped_collision = 0;
  int debug_fit_count = 0;

  int box_index = -1;

  //  ПАРАЛЛЕЛЬНЫЙ ЦИКЛ
  // #pragma omp parallel for schedule(dynamic, 4) \
  //         shared(best, best_idx, cur_box_ptr, pal_ptr, zone_ptr,
  //         debug_skipped_placed, debug_skipped_mass, debug_skipped_size,
  //         debug_skipped_height, debug_skipped_collision, debug_fit_count) \
  //         firstprivate(pal_ptr, zone_ptr)
  for (auto *box_ptr : total_boxes) {

    LocalBest local_best;
    box_index++;
    if (box_ptr->placed) {
#pragma omp atomic
      debug_skipped_placed++;
      continue;
    }
    if (box_ptr->mass + pal_ptr->total_mass > pal_ptr->max_mass) {
#pragma omp atomic
      debug_skipped_mass++;
      continue;
    }

    auto get_rotation =
        box_ptr->full_rotateble ? get_correct_full_rotate : get_correct_rotate;
    std::vector<Size> rotate_variant;
    if (auto v = get_rotation(box_ptr->size, zone_ptr->size)) {
      rotate_variant = std::move(*v);
    }
    for (auto rt_vairant : rotate_variant) {
#pragma omp atomic
      if (!can_place_box_height(pal_ptr, zone_ptr, &rt_vairant,
                                box_ptr->temp_pos, box_ptr->ratio)) {
#ifdef DEBUG_PALLET
#pragma omp critical(debug_print)
        {
          cout << "    Rot[" << j << "] (" << w << "x" << h << "x" << d
               << "): НЕТ ОПОРЫ (height_map)" << endl;
        }
#endif
#pragma omp atomic
        debug_skipped_height++;
        continue;
      }
      if (!fits_without_collision(box_ptr->temp_pos, rt_vairant,
                                  pal_ptr->placed_boxes)) {
#ifdef DEBUG_PALLET
#pragma omp critical(debug_print)
        {
          cout << "    Rot[" << j << "] (" << w << "x" << h << "x" << d
               << "): КОЛЛИЗИЯ на pos=(" << out_x << "," << zone_ptr->xyz[1]
               << "," << out_z << ")" << endl;
        }
#endif
#pragma omp atomic
        debug_skipped_collision++;
        continue;
      }

      auto score = access_box_in_zone(zone_ptr, box_ptr, pal_ptr, box_index,
                                      total_boxes);

#ifdef DEBUG_PALLET
#pragma omp critical(debug_print)
      {
        cout << "    Rot[" << j << "] (" << w << "x" << h << "x" << d
             << "): OK! pos=(" << out_x << "," << zone_ptr->xyz[1] << ","
             << out_z << ") ratio=" << ratio << " scores=[" << res[0] << ","
             << res[1] << "," << res[2] << "," << res[3] << "]" << endl;
      }
#endif

      if (score < local_best.score) {
        local_best.score = score;
        local_best.pos = box_ptr->temp_pos;
        local_best.size = rt_vairant;
        local_best.local_any_fit = true;
      }
    }

    if (local_best.local_any_fit) {
#pragma omp atomic
      debug_fit_count++;
      // #pragma omp critical(update_best)
      {
        if (local_best.score < best) {
          best = local_best.score;
          best_idx = box_index;
          cur_box_ptr = box_ptr;
          box_ptr->temp_pos = local_best.pos;
          box_ptr->size = local_best.size;
          DEBUG_LOG_ENDL("  >>> Новый лучший кандидат: Box "
                         << i << " rot=" << local_best_rot << " scores=["
                         << local_best[0] << "," << local_best[1] << ","
                         << local_best[2] << "]");
        }
      }
    }
  }

  DEBUG_LOG_ENDL("\n--- СТАТИСТИКА ПОИСКА ---");
  DEBUG_LOG_ENDL("Пропущено (уже placed): " << debug_skipped_placed);
  DEBUG_LOG_ENDL("Пропущено (превышение массы): " << debug_skipped_mass);
  DEBUG_LOG_ENDL("Пропущено (не влезает в зону): " << debug_skipped_size);
  DEBUG_LOG_ENDL("Пропущено (нет опоры/height_map): " << debug_skipped_height);
  DEBUG_LOG_ENDL("Пропущено (коллизия): " << debug_skipped_collision);
  DEBUG_LOG_ENDL("Подошли хотя бы в одном повороте: " << debug_fit_count);

  if (best_idx == -1) {
    DEBUG_LOG_ENDL(
        ">>> РЕЗУЛЬТАТ: НЕ НАЙДЕНО подходящей коробки для этой зоны!");
    pal_ptr->failed_in_row++;
    return nullptr;
  }

  DEBUG_LOG_ENDL(">>> РЕЗУЛЬТАТ: Выбрана Box "
                 << best_idx << " размер=("
                 << cur_box_ptr->xyz_size[cur_box_ptr->rotate][0] << "x"
                 << cur_box_ptr->xyz_size[cur_box_ptr->rotate][1] << "x"
                 << cur_box_ptr->xyz_size[cur_box_ptr->rotate][2]
                 << ") rot=" << cur_box_ptr->rotate);

  place_box(pal_ptr, zone_ptr, cur_box_ptr, total_boxes);
  return cur_box_ptr;
}

bool can_place_box_in_zone(Zone *zone, int w, int h, int d) {
  DEBUG_LOG_ENDL("Проверяю можно ли поместить коробку размером ("
                 << w << ", " << h << ", " << d << ") в зону размером ("
                 << zone->xyz_size[0] << ", " << zone->xyz_size[1] << ", "
                 << zone->xyz_size[2] << ")");
  DEBUG_LOG_ENDL("Координаты зоны (" << zone->xyz[0] << ", " << zone->xyz[1]
                                     << ", " << zone->xyz[2] << ")");
  if (w <= zone->size.width && h <= zone->size.height &&
      d <= zone->size.depth) {
    DEBUG_LOG_ENDL("Проверил, что коробка помещается");
    return true;
  }
  DEBUG_LOG_ENDL("Проверил, что коробка НЕ помещается");

  return false;
}
