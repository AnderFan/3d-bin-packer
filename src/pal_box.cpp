#include "pal_box.hpp"
#include "data.hpp"
#include "pal_meb.hpp"
#include "types.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <iostream>
#include <omp.h>
#include <optional>
#include <ostream>
#include <vector>

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

  int new_cx, new_cy, new_cz;

  if (prev_mass == 0) {
    // Если на паллете ничего не было – центр масс = центр этой коробки
    new_cx = cx_box;
    new_cy = cy_box;
    new_cz = cz_box;
  } else {
    new_cx =
        (pal_ptr->mass_centre.x * prev_mass + cx_box * box_mass) / new_mass;
    new_cy =
        (pal_ptr->mass_centre.y * prev_mass + cy_box * box_mass) / new_mass;
    new_cz =
        (pal_ptr->mass_centre.z * prev_mass + cz_box * box_mass) / new_mass;
  }

  res.pos = {new_cx, new_cy, new_cz};

  return res;
}

void center_mass_calculate(Pallet *pal_ptr, Box *box_ptr) {

  int bx = box_ptr->pos.x;
  int by = box_ptr->pos.y;
  int bz = box_ptr->pos.z;

  int bw = box_ptr->size.width;
  int bh = box_ptr->size.height;
  int bd = box_ptr->size.depth;

  // геометрический центр коробки
  int cx_box = bx + bw / 2;
  int cy_box = by + bh / 2;
  int cz_box = bz + bd / 2;

  CenterMassResult cm =
      simulate_center_mass(pal_ptr, box_ptr->mass, cx_box, cy_box, cz_box);

  pal_ptr->total_mass = cm.total_mass;
  pal_ptr->mass_centre = {cm.pos.x, cm.pos.y, cm.pos.z};
}

std::array<int, SCORES_NUM> access_box_in_zone(Zone *zone_ptr, Box *box,
                                               Packing::Vector3 pos, Size size,
                                               Pallet *pal_ptr, int index,
                                               std::vector<Box *> &total_boxes,
                                               Setting &setting) {

  int com_y_score = INT_MAX;
  int com_center_score = INT_MAX;
  int box_center_score = INT_MAX;
  if (setting.center_mass_or_max_volume == 0) { // Если укладка по центру масс
    int cx_box = pos.x + size.width / 2;
    int cy_box = pos.y + size.height / 2;
    int cz_box = pos.z + size.depth / 2;

    CenterMassResult cm =
        simulate_center_mass(pal_ptr, box->mass, cx_box, cy_box, cz_box);
    // идеальный центр масс паллета

    // отклонение от центра по XZ
    int dx = cm.pos.x - pal_ptr->ideal_pos.x;
    // double dy = cm.cy - pal_ptr->ideal_cy;
    int dz = cm.pos.z - pal_ptr->ideal_pos.z;
    int dist2_xz = dx * dx + dz * dz;

    int bdx = cx_box - pal_ptr->ideal_pos.x;
    int bdz = cz_box - pal_ptr->ideal_pos.z;
    int bdist2_xz = bdx * bdx + bdz * bdz;

    com_y_score =
        static_cast<int>(std::round(cm.pos.y * 100)); // ниже центр масс по Y
    com_center_score =
        static_cast<int>(std::round(dist2_xz * 100.0)); // ближе к центру по XZ
    box_center_score = static_cast<int>(std::round(bdist2_xz * 100.0));
  }

  int w_zone = zone_ptr->size.width - (pos.x - zone_ptr->pos.x);
  int y_zone = zone_ptr->size.height - (pos.y - zone_ptr->pos.y);
  int d_zone = zone_ptr->size.depth - (pos.z - zone_ptr->pos.z);

  int height_diff = pos.y + size.height;
  if (setting.center_mass_or_max_volume ==
      1) { // Если укладка по максимальному объему
    if ((pal_ptr->size.width / size.width) *
            (pal_ptr->size.depth / size.depth) <
        (pal_ptr->size.width / size.depth) *
            (pal_ptr->size.depth / size.width)) {
      height_diff += 1000;
    }

    if (!pal_ptr->placed_boxes.empty()) {
      auto prev_box = pal_ptr->placed_boxes[pal_ptr->placed_boxes.size() - 1];
      if (prev_box->size == size) {
        height_diff += 500; // если предыдущая коробка была такого же
                            // но в другой ориентации, штрафуем
      }
    }
  }

  int waste = w_zone * d_zone - size.width * size.depth;
  int long_side = (std::max)(w_zone - size.width, d_zone - size.depth);
  int short_side = (std::min)(w_zone - size.width, d_zone - size.depth);

  if (box->ratio < 0.8)
    height_diff += 1000; // если ratio меньше 0.8, штрафуем
  if (size.height > size.width && size.height > size.depth)
    height_diff += 200;         // преиущественно коробки должны ложиться плашмя
  height_diff += (index * 100); // Коробки отсортированы по убыванию. Чем больше
                                // индекс у коробки тем она меньше

  if (setting.center_mass_or_max_volume == 0) { // Если укладка по центру масс
    return std::array<int, SCORES_NUM>{
        com_center_score, com_y_score, box_center_score,
        height_diff,      waste,       long_side};
  } else if (setting.center_mass_or_max_volume ==
             1) { // Если укладка по максимальному объему
    return std::array<int, SCORES_NUM>{height_diff, waste,   long_side,
                                       short_side,  INT_MAX, INT_MAX};
  }
}

bool can_place_box_height(Pallet *pal_ptr, Zone *zone, Size *box_size,
                          Packing::Vector3 &temp_pos, double &ratio,
                          Setting &setting) {
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
  int min_support_pixels = (setting.min_support * total_area);

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
bool is_placement_possible(Pallet *pallet_ptr, Setting setting,
                           std::vector<Box *> &total_boxes) {
  bool flag_zone = false, flag_box = false;
  if (pallet_ptr->zone_vector.size() > 0) {
    flag_zone = true;
  } // если есть хоть одна живая зона

  if (setting.hMaxQtyCheck ==
      false) { // если кол-во коробок ограничено, то проверяем есть ли
               // неразмещённые коробки
    for (int i = 0; i < total_boxes.size(); i++) {
      if (total_boxes[i]->placed == false) {
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
    return nullptr;
  }

  return best_box_ptr;
}

void place_box(Pallet *pal_ptr, Zone *zone_ptr, Box *box_ptr,
               std::vector<Box *> &total_boxes) {
  pal_ptr->placed_boxes.push_back(box_ptr);

  pal_ptr->max_height_box =
      std::max(pal_ptr->max_height_box, box_ptr->pos.y + box_ptr->size.height);

  total_boxes.erase(remove(total_boxes.begin(), total_boxes.end(), box_ptr),
                    total_boxes.end());
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

  for (int x = x_start; x < x_end; ++x) {
    for (int z = z_start; z < z_end; ++z) {
      pal_ptr->height_map[x][z] =
          std::max(pal_ptr->height_map[x][z], new_height);
    }
  }

  center_mass_calculate(pal_ptr, box_ptr);
}
std::optional<std::vector<Size>> get_correct_full_rotate(Size box, Size zone) {
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
      {{box.width, box.height, box.depth}, {box.depth, box.height, box.width}}};

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
Box *box_placement_handle(Pallet *pal_ptr, Zone *zone_ptr, Setting &setting,
                          std::vector<Box *> &total_boxes) {
  int box_index = -1;

  PlacementCandidate best_candidate;

  for (auto *box_ptr : total_boxes) {

    box_index++;
    if (box_ptr->placed) {
      continue;
    }
    if (box_ptr->mass + pal_ptr->total_mass > pal_ptr->max_mass) {
      continue;
    }
    auto get_rotation =
        box_ptr->full_rotateble ? get_correct_full_rotate : get_correct_rotate;
    std::vector<Size> rotate_variant;
    if (auto v = get_rotation(box_ptr->size, zone_ptr->size)) {
      rotate_variant = std::move(*v);
    }

    for (auto &rt_vairant : rotate_variant) {
      Packing::Vector3 candidate_pos{};
      double cal_ratio = 1.0;
      if (!can_place_box_height(pal_ptr, zone_ptr, &rt_vairant, candidate_pos,
                                box_ptr->ratio, setting)) {
        continue;
      }
      if (!fits_without_collision(candidate_pos, rt_vairant,
                                  pal_ptr->placed_boxes)) {
        continue;
      }

      auto score =
          access_box_in_zone(zone_ptr, box_ptr, candidate_pos, rt_vairant,
                             pal_ptr, box_index, total_boxes, setting);

      if (score < best_candidate.score) {
        best_candidate = PlacementCandidate{box_ptr, candidate_pos, rt_vairant,
                                            cal_ratio, score};
      }
    }
  }
  if (!best_candidate.box) {
    pal_ptr->failed_in_row++;
    return nullptr;
  }
  best_candidate.box->pos = best_candidate.pos;
  best_candidate.box->size = best_candidate.size;
  best_candidate.box->ratio = best_candidate.ratio;

  place_box(pal_ptr, zone_ptr, best_candidate.box, total_boxes);
  return best_candidate.box;
}

bool can_place_box_in_zone(Zone *zone, int w, int h, int d) {
  if (w <= zone->size.width && h <= zone->size.height &&
      d <= zone->size.depth) {
    return true;
  }
  return false;
}
