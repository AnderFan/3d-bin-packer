#include "pal_meb.hpp"
#include "data.hpp"
#include "pal_zone.hpp"
#include "types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <utility>

struct AABB {
  Packing::Vector3 pos;
  Size size;
};

void meb_gen(Pallet *pal_ptr) {
  replace_zones_with_meb(pal_ptr); // ← MEB-регенерация
  pal_ptr->placed_since_meb = 0;
  pal_ptr->failed_in_row = 0;
  pal_ptr->was_defrag = true;
  std::cout << "Пошла дефрагация";
}

// void check_meb(pallet* pal_ptr, vector<box*> total_boxes) {
//	int SUCCESS_PLACE = total_boxes.size() < 20 ? 10 : total_boxes.size() /
// 2; 	int FAILER_PLACE = total_boxes.size() < 20 ? 5 : total_boxes.size() /
// 3;;
//
//	if ((pal_ptr->placed_since_meb >= SUCCESS_PLACE ||
// pal_ptr->failed_in_row >= FAILER_PLACE) && pal_ptr->was_defrag == false) {
//		meb_gen(pal_ptr);
//	}
//
// }

void check_meb(Pallet *pal_ptr, const std::vector<Box *> &total_boxes) {
  int SUCCESS_PLACE =
      total_boxes.size() < 20 ? 10 : (int)total_boxes.size() / 2;
  int FAILER_PLACE = total_boxes.size() < 20 ? 5 : (int)total_boxes.size() / 3;

  if ((pal_ptr->placed_since_meb >= SUCCESS_PLACE ||
       pal_ptr->failed_in_row >= FAILER_PLACE) &&
      pal_ptr->was_defrag == false) {
    meb_gen(pal_ptr);
  }
}

inline bool aabb_intersect(const AABB &a, const AABB &b) {
  return !(
      a.pos.x + a.size.width <= b.pos.x || b.pos.x + b.size.width <= a.pos.x ||
      a.pos.y + a.size.height <= b.pos.y ||
      b.pos.y + b.size.height <= a.pos.y || a.pos.z + a.size.depth <= b.pos.z ||
      b.pos.z + b.size.depth <= a.pos.z);
}

inline bool aabb_empty(const AABB &a) {
  return a.size.width <= 0 || a.size.depth <= 0 || a.size.height <= 0;
}

// true = ВНУТРИ этой ячейки нет ни одной уложенной коробки
bool empty_of_boxes(const AABB &cell, const std::vector<Box *> &placed) {
  for (auto *b : placed) {
    AABB bb{b->pos, b->size};
    if (aabb_intersect(cell, bb))
      return false; // любая коллизия → не пусто
  }
  return true;
}

static void uniq_sort(std::vector<int> &v) {
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
}

AABB grow_meb_from_node(int xi, int yi, int zi, const std::vector<int> &Xs,
                        const std::vector<int> &Ys, const std::vector<int> &Zs,
                        const std::vector<Box *> &placed) {
  // Границы индексов
  if (xi + 1 >= (int)Xs.size() || yi + 1 >= (int)Ys.size() ||
      zi + 1 >= (int)Zs.size()) {
    return AABB{0, 0, 0, 0, 0, 0};
  }

  // 1) минимальная ячейка
  AABB cur{{Xs[xi], Ys[yi], Zs[zi]},
           {Xs[xi + 1] - Xs[xi], Ys[yi + 1] - Ys[yi], Zs[zi + 1] - Zs[zi]}};

  if (aabb_empty(cur) || !empty_of_boxes(cur, placed)) {
    return AABB{{0, 0, 0}, {0, 0, 0}};
  }

  // 2) Растяжка по X: пробуем растянуть вправо до каждого следующего координата
  for (int xj = xi + 2; xj < (int)Xs.size();
       ++xj) { // ← ПРАВИЛЬНО: xj идёт от xi+2
    AABB test = cur;
    test.size.width = Xs[xj] - cur.pos.x; // растянули до Xs[xj]
    if (!empty_of_boxes(test, placed))
      break;    // упёрлись в коробку
    cur = test; // приняли растяжку
  }

  // 3) растяжка по Y
  for (int yj = yi + 2; yj < (int)Ys.size(); ++yj) {
    AABB test = cur;
    test.size.height = Ys[yj] - cur.pos.y;
    if (!empty_of_boxes(test, placed))
      break;
    cur = test;
  }

  // 4) растяжка по Z
  for (int zj = zi + 2; zj < (int)Zs.size(); ++zj) {
    AABB test = cur;
    test.size.depth = Zs[zj] - cur.pos.z;
    if (!empty_of_boxes(test, placed))
      break;
    cur = test;
  }

  return cur;
}

// Строим новый список зон (zone*) из MEB-боксов на основании УЖЕ уложенных
// коробок. Эти зоны заменят текущий zone_vector → «дефрагментация».
std::vector<Zone *> build_meb_zones(Pallet *pal) {
  std::vector<int> Xs{
      0, pal->size.width}; // Например, если паллета по X имеет длину 800 мм, и
                           // у тебя стоят коробки: первая от x=0 до x=400,
                           // вторая от x=400 до x=800, то XS  { 0, 400, 800}
  std::vector<int> Ys{0, pal->size.height};
  std::vector<int> Zs{0, pal->size.depth};

  for (auto *b : pal->placed_boxes) {
    Xs.push_back(b->pos.x);
    Xs.push_back(b->pos.x + b->size.width);
    Ys.push_back(b->pos.y);
    Ys.push_back(b->pos.y + b->size.height);
    Zs.push_back(b->pos.z);
    Zs.push_back(b->pos.z + b->size.depth);
  }
  uniq_sort(Xs);
  uniq_sort(Ys);
  uniq_sort(Zs);

  // 2) Растяжка из каждого узла сетки
  std::vector<Zone *> out;
  out.reserve((Xs.size() - 1) * (Ys.size() - 1) * (Zs.size() - 1) /
              8); // грубая оценка

  for (int xi = 0; xi < (int)Xs.size() - 1; ++xi)
    for (int yi = 0; yi < (int)Ys.size() - 1; ++yi)
      for (int zi = 0; zi < (int)Zs.size() - 1; ++zi) {
        AABB meb =
            grow_meb_from_node(xi, yi, zi, Xs, Ys, Zs, pal->placed_boxes);
        if (!aabb_empty(meb)) {
          // Преобразуем AABB в zone
          out.push_back(new Zone{
              {meb.pos.x, meb.pos.y, meb.pos.z},                 // xyz
              {meb.size.width, meb.size.height, meb.size.depth}, // xyz_size
              true                                               // usable
          });
        }
      }
  return out;
}
bool fits_without_collision(Packing::Vector3 temp_pos, Size box_size,
                            const std::vector<Box *> &placed) {
  AABB cand{temp_pos, box_size};
  for (auto *b : placed) {
    AABB bb{b->pos, b->size};
    if (aabb_intersect(cand, bb))
      return false;
  }
  return true;
}
