#include "data.hpp"
#include "pal_meb.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <utility> // std::move

static void dedup_ptrs(std::vector<Zone *> &v) { // убираем nullptr и дубли по
                                                 // адресу из вектора зон
  // убираем nullptr на всякий случай
  v.erase(std::remove(v.begin(), v.end(), nullptr), v.end());

  // дедуп по адресу
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
}

// bool is_zone_sane(
//     const Pallet *pal,
//     const Zone *z) { // проверяем зону на адекватность, чтобы не было
//           z          // нулл поинтеров, отрицательных размеров и т.д.
//   if (!z || !z->usable)
//     return false;
//
//   // размеры должны быть > 0 и не превышать паллету
//   if (z->size.width <= 0 || z->size.height <= 0 || z->size.depth <= 0)
//     return false;
//   if (z->pos.x < 0 || z->pos.y < 0 || z->pos.z < 0)
//     return false;
//
//   // зона должна помещаться в паллету (с допуском)
//   if (z->pos.x + z->size.width > pal->size.width)
//     return false;
//   if (z->pos.y + z->size.height > pal->size.height)
//     return false;
//   if (z->pos.z + z->size.depth > pal->size.depth)
//     return false;
//
//   return true;
// }
bool is_zone_sane(const Pallet *pal, const Zone *z) {
  if (!pal || !z) {
    std::cerr << "[REJECT] Null pointer: pal=" << pal << ", z=" << z << "\n";
    return false;
  }
  if (!z->usable) {
    std::cerr << "[REJECT] Zone is NOT usable!\n";
    return false;
  }
  if (z->size.width <= 0 || z->size.height <= 0 || z->size.depth <= 0) {
    std::cerr << "[REJECT] Non-positive size: (" << z->size.width << ", "
              << z->size.height << ", " << z->size.depth << ")\n";
    return false;
  }
  if (z->pos.x < 0 || z->pos.y < 0 || z->pos.z < 0) {
    std::cerr << "[REJECT] Negative pos: (" << z->pos.x << ", " << z->pos.y
              << ", " << z->pos.z << ")\n";
    return false;
  }
  if (z->pos.x + z->size.width > pal->size.width ||
      z->pos.y + z->size.height > pal->size.height ||
      z->pos.z + z->size.depth > pal->size.depth) {
    std::cerr << "[REJECT] Out of pallet bounds!\n"
              << "  Zone bounds: X=" << z->pos.x + z->size.width
              << ", Y=" << z->pos.y + z->size.height
              << ", Z=" << z->pos.z + z->size.depth << "\n"
              << "  Pallet size: W=" << pal->size.width
              << ", H=" << pal->size.height << ", D=" << pal->size.depth
              << "\n";
    return false;
  }
  return true;
}
static void validate_zone_vector(
    const Pallet
        *pal) { // вызываем эту функцию после всех операций с зонами, чтобы
                // убедиться что мы не создали какую то коррумпированную зону
#ifndef NDEBUG
  const auto &v = pal->zone_vector;
  for (size_t i = 0; i < v.size(); ++i) {
    if (!is_zone_sane(pal, v[i])) {
      std::cerr << "CORRUPT ZONE at index " << i << " ptr=" << v[i] << "\n";
    }
  }
#endif
}

Zone *select_zone(Pallet *pallet_ptr) {
  Zone *best = nullptr;
  std::array<int, 2> best_score = {INT_MAX, INT_MAX};
  for (auto *z : pallet_ptr->zone_vector) { // Проходка по всем зона
    if (!z->usable)
      continue;
    int vol =
        z->size.width * z->size.height * z->size.depth; // высчитываем объём
    std::array<int, 2> score = {
        z->pos.y, vol}; // подсчитываем счёт, если объём одинаковый, то мы
                        // будем выбирать ту зону, что ниже
    if (score < best_score) {
      best_score = score;
      best = z;
    }
  }
  return best;
}

// void kill_zone(pallet* pallet_ptr, zone* zone) {
//	pallet_ptr->zone_vector.erase(
//		remove(pallet_ptr->zone_vector.begin(),
// pallet_ptr->zone_vector.end(), zone),
// pallet_ptr->zone_vector.end()
//	);
//
//  }

static void unlink_zone(Pallet *pallet_ptr, Zone *z) {
  if (!pallet_ptr || !z)
    return;
  auto &v = pallet_ptr->zone_vector;
  v.erase(std::remove(v.begin(), v.end(), z), v.end());
}

void kill_zone(Pallet *pallet_ptr, Zone *z) {
  if (!pallet_ptr || !z)
    return;
  unlink_zone(pallet_ptr, z);

  auto &dead = pallet_ptr->zone_dead_vector;
  if (std::find(dead.begin(), dead.end(), z) != dead.end()) {
    return; // Если зона уже помечена как dead, не вызываем delete повторно!
  }

  delete z;
}

void sort_by_xyz_then_size(
    std::vector<Zone *> &zones) { // сортируем зоны по координатам и размерам
  std::sort(zones.begin(), zones.end(), [](Zone *a, Zone *b) {
    if (a->pos.y != b->pos.y)
      return a->pos.y < b->pos.y; // Сначала Y
    if (a->pos.x != b->pos.x)
      return a->pos.x < b->pos.x; // потом X
    if (a->pos.z != b->pos.z)
      return a->pos.z < b->pos.z; // потом Z
    if (a->size.width != b->size.width)
      return a->size.width < b->size.width; // потом ширина
    if (a->size.height != b->size.height)
      return a->size.height < b->size.height; // потом глубина
    return a->size.depth < b->size.depth;     // потом высота
  });
}

bool zones_equal(const Zone *a, const Zone *b) { // проверяем равенство зон
  return a->pos.x == b->pos.x && a->pos.y == b->pos.y && a->pos.z == b->pos.z &&
         a->size.width == b->size.width && a->size.height == b->size.height &&
         a->size.depth == b->size.depth;
}

bool intersect(const Zone *a, const Zone *b, Zone &I) {
  int x1 = std::max(a->pos.x, b->pos.x); // Пересечение начинается там, где
                                         // начинается «позже» из двух зон;
  int y1 =
      std::max(a->pos.y, b->pos.y); // то есть — правее из двух левых граней.
  int z1 = std::max(a->pos.z, b->pos.z);

  int x2 = std::min(a->pos.x + a->size.width,
                    b->pos.x + b->size.width); // правая грань пересечения
  int y2 = std::min(
      a->pos.y + a->size.height,
      b->pos.y + b->size.height); // это самая левая из правых граней двух зон
  int z2 = std::min(a->pos.z + a->size.depth, b->pos.z + b->size.depth);

  if (x2 <= x1 || y2 <= y1 || z2 <= z1)
    return false; // нет пересечения

  I.pos = {x1, y1, z1};
  I.size = {(x2 - x1), (y2 - y1), (z2 - z1)};
  return true; // Довольно сложная логика если не знать математику, помогал джпт
}

std::vector<Zone *>
subtract(const Zone *A,
         const Zone &I) { // Создаёт куски зоны A без пересечения с I.
  std::vector<Zone *> out;
  // Замените начало функции subtract на явное присвоение без двусмысленности:
  int ax = A->pos.x, ay = A->pos.y, az = A->pos.z;
  int aw = A->size.width, ah = A->size.height, ad = A->size.depth;
  int ax2 = ax + aw, ay2 = ay + ah, az2 = az + ad;

  int ix = I.pos.x, iy = I.pos.y, iz = I.pos.z;
  int iw = I.size.width, ih = I.size.height, id = I.size.depth;
  int ix2 = ix + iw, iy2 = iy + ih, iz2 = iz + id;

  auto add = [&](int x, int y, int z, int w, int h, int d) {
    if (w <= 0 || h <= 0 || d <= 0)
      return;
    out.push_back(new Zone{{x, y, z}, {w, h, d}, true});
  };

  // И далее аккуратно передавайте размеры в порядке (x, y, z, w, h, d):
  if (ix > ax)
    add(ax, ay, az, ix - ax, ah, ad);
  if (ix2 < ax2)
    add(ix2, ay, az, ax2 - ix2, ah, ad);
  if (iy > ay)
    add(std::max(ax, ix), ay, az, std::min(ax2, ix2) - std::max(ax, ix),
        iy - ay, ad);
  if (iy2 < ay2)
    add(std::max(ax, ix), iy2, az, std::min(ax2, ix2) - std::max(ax, ix),
        ay2 - iy2, ad);
  if (iz > az)
    add(std::max(ax, ix), std::max(ay, iy), az,
        std::min(ax2, ix2) - std::max(ax, ix),
        std::min(ay2, iy2) - std::max(ay, iy), iz - az);
  if (iz2 < az2)
    add(std::max(ax, ix), std::max(ay, iy), iz2,
        std::min(ax2, ix2) - std::max(ax, ix),
        std::min(ay2, iy2) - std::max(ay, iy), az2 - iz2);
  return out;
}
void sub_zone(Pallet *pal_ptr) {
  // Разрезаем пересекающиеся зоны

  dedup_ptrs(pal_ptr->zone_vector);

  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t i = 0; i < pal_ptr->zone_vector.size() && !changed; ++i) {
      for (size_t j = i + 1; j < pal_ptr->zone_vector.size(); j++) {
        if (pal_ptr->zone_vector[i] == pal_ptr->zone_vector[j])
          continue;
        Zone I{};
        if (intersect(pal_ptr->zone_vector[i], pal_ptr->zone_vector[j], I)) {
          auto parts = subtract(pal_ptr->zone_vector[i], I);
          delete pal_ptr->zone_vector[i];
          pal_ptr->zone_vector.erase(pal_ptr->zone_vector.begin() + i);
          pal_ptr->zone_vector.insert(pal_ptr->zone_vector.begin() + i,
                                      parts.begin(), parts.end());
          changed = true;
          break;
        }
      }
    }
  }
}

bool contained(const Zone *a, const Zone *b) {
  return b->pos.x <= a->pos.x && b->pos.y <= a->pos.y && b->pos.z <= a->pos.z &&
         a->pos.x + a->size.width <= b->pos.x + b->size.width &&
         a->pos.y + a->size.height <= b->pos.y + b->size.height &&
         a->pos.z + a->size.depth <= b->pos.z + b->size.depth;
}

void remove_contained(
    std::vector<Zone *>
        &zs) { // Удаляет из zs зоны, полностью содержащиеся в других зонах.
  dedup_ptrs(zs);

  for (size_t i = 0; i < zs.size(); ++i) {
    for (size_t j = 0; j < zs.size(); ++j) {
      if (i == j)
        continue;
      if (contained(zs[i], zs[j])) {
        delete zs[i];
        zs.erase(zs.begin() + i);
        --i;
        break;
      }
    }
  }

  dedup_ptrs(zs);
}

void clipping(Pallet *pal_ptr) {
  dedup_ptrs(pal_ptr->zone_vector);

  auto clip = [&](Zone *zone) {
    auto &[x, y, z] = zone->pos;
    auto &[sx, sy, sz] = zone->size;

    int X2 = std::min(x + sx, pal_ptr->size.width);
    int Y2 = std::min(y + sy, pal_ptr->size.height);
    int Z2 = std::min(z + sz, pal_ptr->size.depth);

    x = std::max(x, 0);
    y = std::max(y, 0);
    z = std::max(z, 0);

    sz = Z2 - z;
    sx = X2 - x;
    sy = Y2 - y;
  };
  for (auto *zone : pal_ptr->zone_vector)
    clip(zone);

  auto &v = pal_ptr->zone_vector;
  auto it = std::remove_if(v.begin(), v.end(), [](Zone *zone) {
    return zone->size.width <= 0 || zone->size.height <= 0 ||
           zone->size.depth <= 0;
  });

  for (auto p = it; p != v.end(); ++p)
    delete *p;
  v.erase(it, v.end());
}

bool can_mergeX(const Zone *A, const Zone *B) {
  return A->pos.y == B->pos.y && A->pos.z == B->pos.z &&
         A->size.height == B->size.height && A->size.depth == B->size.depth &&
         (A->pos.x + A->size.width == B->pos.x ||
          B->pos.x + B->size.width == A->pos.x);
}

Zone *mergeX(const Zone *A, const Zone *B) {
  int x = std::min(A->pos.x, B->pos.x);
  int y = A->pos.y;
  int z = A->pos.z;
  int w = A->size.width + B->size.width;
  int h = A->size.height;
  int d = A->size.depth;
  return new Zone{{x, y, z}, {w, h, d}, true};
}
inline bool can_mergeY(const Zone *A, const Zone *B) {
  // Совпадают по X и Z-полосе и размерам по X,Z
  bool same_band = A->pos.x == B->pos.x &&           // x
                   A->pos.z == B->pos.z &&           // z
                   A->size.width == B->size.width && // w
                   A->size.depth == B->size.depth;   // h

  if (!same_band)
    return false;
  // Касаются по целой грани вдоль Y (без зазора и без перекрытия):
  // ...A...|B  или  B|...A...
  bool touch = (A->pos.y + A->size.height == B->pos.y) ||
               (B->pos.y + B->size.height == A->pos.y);

  return touch;
}

inline Zone *mergeY(const Zone *A, const Zone *B) {
  // Предполагается, что can_mergeY(A,B) == true
  int x = A->pos.x;
  int z = A->pos.z;
  int y = std::min(A->pos.y, B->pos.y);    // нижняя грань по Y
  int w = A->size.width;                   // одинаковые у A и B
  int d = A->size.depth;                   // одинаковые у A и B
  int h = A->size.height + B->size.height; // суммируем глубину (по Y)

  return new Zone{{x, y, z}, {w, h, d}, true};
}

inline bool can_mergeZ(const Zone *A, const Zone *B) {
  // Совпадают по X и Y-полосе и размерам по X,Y
  bool same_band = A->pos.x == B->pos.x &&           // x
                   A->pos.y == B->pos.y &&           // y
                   A->size.width == B->size.width && // w
                   A->size.height == B->size.height; // h

  if (!same_band)
    return false;

  bool touch = (A->pos.z + A->size.depth == B->pos.z) ||
               (B->pos.z + B->size.depth == A->pos.z);

  return touch;
}

inline Zone *mergeZ(const Zone *A, const Zone *B) {
  // Предполагается, что can_mergeZ(A,B) == true
  int x = A->pos.x;
  int y = A->pos.y;
  int z = std::min(A->pos.z, B->pos.z);  // нижняя грань по Z
  int w = A->size.width;                 // одинаковые у A и B
  int h = A->size.height;                // одинаковые у A и B
  int d = A->size.depth + B->size.depth; // суммируем высоту (по Z)

  return new Zone{{x, y, z}, {w, h, d}, true};
}
bool try_merge_once(std::vector<Zone *> &zs) {
  dedup_ptrs(zs);

  for (size_t i = 0; i < zs.size(); ++i) {
    for (size_t j = i + 1; j < zs.size(); ++j) {
      Zone *A = zs[i], *B = zs[j];
      Zone *M = nullptr;

      if (can_mergeX(A, B))
        M = mergeX(A, B);
      else if (can_mergeY(A, B))
        M = mergeY(A, B);
      else if (can_mergeZ(A, B))
        M = mergeZ(A, B);

      if (M) {
        delete A;
        delete B;
        zs.erase(zs.begin() + j);
        zs.erase(zs.begin() + i);
        zs.insert(zs.begin() + i, M);
        return true; // сделали одно слияние — выходим, начнём заново
      }
    }
  }
  return false;
}
void merge_zone(std::vector<Zone *> &pal_ptr) {
  sort_by_xyz_then_size(pal_ptr);

  bool changed = true;
  while (changed)
    changed = try_merge_once(pal_ptr);
}

void erase_dub(Pallet *pal_ptr) {
  auto &v = pal_ptr->zone_vector;

  dedup_ptrs(v);
  sort_by_xyz_then_size(v);

  auto it = std::unique(v.begin(), v.end(), zones_equal);
  for (auto d = it; d != v.end(); ++d)
    delete *d;
  v.erase(it, v.end());
}
void split_zone(Pallet *pallet_ptr, Zone *zone_to_split_pointer, Box *box_ptr) {
  // Сохраняем данные зоны ДО удаления
  const auto [zx, zy, zz] = zone_to_split_pointer->pos;
  const auto [zsx, zsy, zsz] = zone_to_split_pointer->size;
  const int zx2 = zx + zsx;
  const int zy2 = zy + zsy;
  const int zz2 = zz + zsz;

  // Данные коробки
  const auto [bx, by, bz] = box_ptr->pos;
  const auto [bsx, bsy, bsz] = box_ptr->size;
  const int bx2 = bx + bsx;
  const int by2 = by + bsy;
  const int bz2 = bz + bsz;

  // Удаляем старую зону
  zone_to_split_pointer->usable = false;
  kill_zone(pallet_ptr, zone_to_split_pointer);

  auto add_zone = [&](int ax, int ay, int az, int asx, int asy, int asz) {
    if (asx <= 0 || asy <= 0 || asz <= 0)
      return;
    if (ax < 0 || ay < 0 || az < 0) {
      std::cout << "CRITICAL ERROR: Zone at negative coords: " << ax << ", "
                << ay << ", " << az << std::endl;
      return;
    }
    pallet_ptr->zone_vector.push_back(
        new Zone{{ax, ay, az}, {asx, asy, asz}, true});
  };

  // GUILLOTINE SPLIT (непересекающиеся зоны)

  // 1. Справа (по X): от правой грани коробки до правой грани зоны
  //    Занимает ВСЮ высоту и глубину ЗОНЫ (не коробки!)
  add_zone(bx2, zy, zz, zx2 - bx2, zsy, zsz);

  // 2. Сверху (по Y): от верха коробки до верха зоны
  //    Ширина ограничена коробкой (справа уже занято зоной 1)
  //    Глубина - вся глубина зоны
  add_zone(bx, by2, zz, bsx, zy2 - by2, zsz);

  // 3. Сзади (по Z): от задней грани коробки до конца зоны
  //    Ширина и высота ограничены коробкой (остальное занято зонами 1 и 2)
  add_zone(bx, by, bz2, bsx, bsy, zz2 - bz2);
}
// }

bool merge_any_pair_XYZ(
    Pallet *pal) { // попытка чё-нибудь слить, что бы не расстраиваться
  auto &zs = pal->zone_vector;

  dedup_ptrs(zs);
  sort_by_xyz_then_size(zs);

  for (size_t i = 0; i < zs.size(); ++i) {
    for (size_t j = i + 1; j < zs.size(); ++j) {
      Zone *A = zs[i];
      Zone *B = zs[j];
      Zone *M = nullptr;

      if (can_mergeX(A, B))
        M = mergeX(A, B);
      else if (can_mergeY(A, B))
        M = mergeY(A, B);
      else if (can_mergeZ(A, B))
        M = mergeZ(A, B);

      if (M) {
        delete A;
        delete B;
        zs.erase(zs.begin() + j);
        zs.erase(zs.begin() + i);
        zs.insert(zs.begin() + i, M);

        sort_by_xyz_then_size(zs);
        remove_contained(zs);
        std::cout << "Слили зону для увеличения шансов размещения коробок"
                  << std::endl;
        return true;
      }
    }
  }
  return false;
}

void zone_cleanup(Pallet *pal_ptr) { // чистим зоны от мусора.

  dedup_ptrs(pal_ptr->zone_vector);

  clipping(pal_ptr);  // обрезаем зоны по размерам паллеты
  erase_dub(pal_ptr); // удаляем дублирующиеся зоны

  dedup_ptrs(pal_ptr->zone_vector);

  // merge_zone(pal_ptr->zone_vector); // Мердж соседних зон

  //// После мерджа опять чистим от мусора
  clipping(pal_ptr);
  remove_contained(pal_ptr->zone_vector);
  sort_by_xyz_then_size(
      pal_ptr->zone_vector); // сортируем зоны по координатам и размерам
  erase_dub(pal_ptr);        // удаляем дублирующиеся зоны
  clipping(pal_ptr);
  dedup_ptrs(pal_ptr->zone_vector);

  validate_zone_vector(pal_ptr);
}

void replace_zones_with_meb(Pallet *pal) {
  auto newZones = build_meb_zones(pal);

  for (auto *z : pal->zone_vector)
    delete z;
  pal->zone_vector.clear();

  pal->zone_vector = std::move(newZones);

  clipping(pal);                           // обрезать по габаритам паллеты
  sort_by_xyz_then_size(pal->zone_vector); // стабильность порядка
  remove_contained(pal->zone_vector); // убрать зоны, полностью внутри других
  erase_dub(pal);                     // убрать точные дубли (с delete хвоста)
  merge_zone(pal->zone_vector);       // (опционально) слить касающиеся блоки
  clipping(pal);                      // финальный safety-pass
}
