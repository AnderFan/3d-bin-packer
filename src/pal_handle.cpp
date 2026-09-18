#include "data.hpp"
#include "pal_box.hpp"
#include "pal_meb.hpp"
#include "pal_zone.hpp"
#include <algorithm>
#include <iostream>
#include <ostream>

void ClearPallet(Pallet &pal, std::vector<Box *> &total_boxes) {
  for (auto *zone : pal.zone_vector) {
    delete zone;
  }
  pal.zone_vector.clear();
  pal.placed_boxes.clear();

  for (auto *b : total_boxes) {
    if (!b)
      continue;
    b->placed = false;
    b->pos = {-1, -1, -1};
    b->scores = {INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX};
  }
  pal.total_mass = 0;
  pal.max_height_box = 0;
  pal.mass_centre = {0, 0, 0};
  pal.zone_vector.push_back(new Zone{
      {0, 0, 0}, {pal.size.width, pal.size.height, pal.size.depth}, true});
}

static void rebuild_pallet_state(Pallet *pal_ptr) {
  if (!pal_ptr)
    return;

  pal_ptr->total_mass = 0;
  pal_ptr->mass_centre = {0, 0, 0};
  pal_ptr->max_height_box = 0;

  height_map_init(pal_ptr);

  for (auto *b : pal_ptr->placed_boxes) {
    if (!b)
      continue;

    // пересчет max height
    pal_ptr->max_height_box =
        std::max(pal_ptr->max_height_box, b->pos.y + b->size.height);

    center_mass_calculate(pal_ptr, b);

    // пересчет height_map (аналогично place_box)
    int x_start = b->pos.x;
    int x_end = x_start + b->size.width;
    int z_start = b->pos.z;
    int z_end = z_start + b->size.depth;
    int new_height = b->pos.y + b->size.height;

    for (int x = x_start; x < x_end; ++x) {
      for (int z = z_start; z < z_end; ++z) {
        pal_ptr->height_map[x][z] =
            std::max(pal_ptr->height_map[x][z], new_height);
      }
    }
  }
}

Box *clone_box(const Box *src) {
  if (!src)
    return nullptr;

  Box *b = new Box(*src); // копирует размеры, массу, full_rotateble и т.д.
  b->placed = false;
  b->pos.x = b->pos.y = b->pos.z = -1;
  b->pos.x = b->pos.y = -1;
  b->scores = {INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX};
  return b;
}

void sort_boxes(std::vector<Box *> &total_boxes) {
  std::sort(total_boxes.begin(), total_boxes.end(),
            [](const Box *a, const Box *b) {
              int vol_a = a->size.width * a->size.depth * a->size.height;
              int vol_b = b->size.depth * b->size.height * b->size.width;

              if (vol_a != vol_b) {
                return vol_a > vol_b; // Сначала большие по объёму
              }

              // При равном объёме — сначала более тяжёлые (для лучшего центра
              // масс)
              return a->mass > b->mass;
            });
}

void centering_box(Pallet *pal_ptr) {
  int max_x = 0, max_z = 0;
  for (const auto &box_ptr :
       pal_ptr->placed_boxes) { // находим макс координаты по x и z среди
                                // размещенных коробок
    int box_x_end = box_ptr->pos.x + box_ptr->size.width;
    int box_z_end = box_ptr->pos.z + box_ptr->size.depth;
    max_x = std::max(max_x, box_x_end);
    max_z = std::max(max_z, box_z_end);
  }

  int indent_x = 0, indent_z = 0;
  if (max_x < pal_ptr->size.width) {
    indent_x = (pal_ptr->size.width - max_x) / 2;
  }
  if (max_z < pal_ptr->size.depth) {
    indent_z = (pal_ptr->size.depth - max_z) / 2;
  }

  for (auto &box_ptr :
       pal_ptr->placed_boxes) { // смещаем все коробки на indent_x и indent_z,
                                // чтобы центрировать по x и z
    box_ptr->pos.x += indent_x;
    box_ptr->pos.z += indent_z;
  }

  pal_ptr->mass_centre.x += indent_x;
  pal_ptr->mass_centre.z += indent_z;
}
void pallet_handle(Pallet *pal_ptr, std::vector<Box *> total_boxes,
                   Setting setting) {
  ClearPallet(*pal_ptr, total_boxes);
  sort_boxes(total_boxes);

  // Если выбран режим Макс  докидывать коробки одного типа бесконечно
  Box *unlimited_template = nullptr;
  if (setting.hMaxQtyCheck && !total_boxes.empty()) {
    unlimited_template =
        total_boxes.front(); // один тип: берем первый как шаблон
  }

  std::cout << "Всего коробок: " << total_boxes.size() << std::endl;
  std::cout << "Кол-во ЗОН " << pal_ptr->zone_vector.size() << std::endl;
  Zone *pal_zone_ptr;
  Box *placed_box_ptr = nullptr;
  height_map_init(pal_ptr);
  std::cout << "Начинаем размещение коробок на паллете" << std::endl;
  ;

  int max_iterations =
      total_boxes.size() * setting.max_iterations; // Лимит итераций
  int iterations = 0;
  int failed_iterations = 0;
  bool last_chance = false;
  while (is_placement_possible(pal_ptr, setting, total_boxes)) {
    iterations++;
    if (iterations > max_iterations) {
      std::cout << "Достигнут лимит итераций, прекращаем укладку" << std::endl;
      break;
    }

    if (setting.hMaxQtyCheck && unlimited_template) {
      // если коробок не осталось
      if (total_boxes.empty()) {
        total_boxes.push_back(clone_box(unlimited_template));
      }
    }

    check_meb(pal_ptr, total_boxes);

    if (pal_ptr->zone_vector.empty()) {
      std::cout << "Нет доступных зон для размещения\n";
      break;
    }

    pal_zone_ptr = select_zone(pal_ptr);

    if (!pal_zone_ptr || !is_zone_sane(pal_ptr, pal_zone_ptr)) {
      std::cout << "Выбранная зона проклята, удаляем её" << std::endl;
      std::cout << "Zone [" << "] "
                << "Pos: (" << pal_zone_ptr->pos.x << ", "
                << pal_zone_ptr->pos.y << ", " << pal_zone_ptr->pos.z << ") | "
                << "Size: (" << pal_zone_ptr->size.width << ", "
                << pal_zone_ptr->size.height << ", " << pal_zone_ptr->size.depth
                << ")\n";
      kill_zone(pal_ptr, pal_zone_ptr);
      failed_iterations++;
      if (failed_iterations > setting.max_failed) {
        std::cout << "Слишком много неудачных итераций, прекращаем укладку"
                  << std::endl;
        break;
      }
      continue;
    }
    if (!pal_zone_ptr->usable)
      continue;

    placed_box_ptr =
        box_placement_handle(pal_ptr, pal_zone_ptr, setting, total_boxes);

    if (placed_box_ptr) {
      split_zone(pal_ptr, pal_zone_ptr, placed_box_ptr);
      zone_cleanup(pal_ptr);
      std::cout << "Зон щас:" << pal_ptr->zone_vector.size() << std::endl;
      std::cout << "Всего размещено коробок: " << pal_ptr->placed_boxes.size()
                << std::endl;
      failed_iterations = 0;
    } else {
      // Размещение не удалось
      if (merge_any_pair_XYZ(pal_ptr)) {
        zone_cleanup(pal_ptr);
        pal_ptr->failed_in_row++;
        failed_iterations++;
        continue;
      }

      std::cout << "Убираем зону из доступных" << std::endl;
      kill_zone(pal_ptr, pal_zone_ptr);
      zone_cleanup(pal_ptr);
      failed_iterations++;

      if (failed_iterations > setting.max_failed) {
        std::cout << "Слишком много неудачных итераций, прекращаем укладку"
                  << std::endl;
        break;
      }

      if (pal_ptr->zone_vector.size() == 0 && last_chance == false) {
        std::cout << "Протокол ПОСЛЕДНИЙ ШАНС" << std::endl;
        last_chance = true;
        meb_gen(pal_ptr);
        continue;
      }
    }
    if (last_chance) {
      break;
    }
  }

  if (setting.lim_lay && setting.hMaxQtyCheck) {
    if (pal_ptr->placed_boxes.empty()) {
      // нечего удалять
    } else {
      int qbox_lay = 0;
      for (auto &box :
           pal_ptr->placed_boxes) { // Считаем кол-во коробок в первом слое
        if (box->pos.y == 0) {
          qbox_lay++;
        }
      }
      if (qbox_lay > 0) {
        int placed = static_cast<int>(pal_ptr->placed_boxes.size());
        int full_layers = placed / qbox_lay;
        int need_box = full_layers * qbox_lay; // оставить только целые слои
        int del_box = placed - need_box;       // удалить только неполный хвост

        for (int i = 0; i < del_box; ++i) {
          Box *b = pal_ptr->placed_boxes.back();
          pal_ptr->placed_boxes.pop_back();
          delete b;
        }
      }
      rebuild_pallet_state(pal_ptr);
    }
  }

  centering_box(pal_ptr);

  std::cout << "\n========== Размещение завершено ==========" << std::endl;
  std::cout << "Всего размещено коробок: " << pal_ptr->placed_boxes.size()
            << std::endl;
  std::cout << "Максимальная высота: " << pal_ptr->max_height_box << " см"
            << std::endl;
  std::cout << "Вес паллеты: " << pal_ptr->total_mass << " кг из "
            << pal_ptr->max_mass << " кг" << std::endl;

  int total_volume =
      pal_ptr->size.width * pal_ptr->size.height * pal_ptr->size.depth;
  int used_volume = 0;

  for (const auto &box_ptr : pal_ptr->placed_boxes) {
    used_volume +=
        box_ptr->size.width * box_ptr->size.height * box_ptr->size.depth;
  }

  std::cout << "Заполнено объема: " << used_volume << " из " << total_volume
            << " (" << (used_volume * 100.0) / total_volume << "%)"
            << std::endl;
  std::cout << "Осталось неразмещенных коробок: " << total_boxes.size()
            << std::endl;

  if (!total_boxes.empty()) {
    std::cout << "\nНеразмещенные коробки:" << std::endl;
    for (auto &box_ptr : total_boxes) {
      if (box_ptr->placed == false) {
        std::cout << "  Размер: (" << box_ptr->size.width << ", "
                  << box_ptr->size.height << ", " << box_ptr->size.depth
                  << ") см, вес: " << box_ptr->mass << " кг" << std::endl;
      }
    }
  }

  if (iterations >= max_iterations) {
    std::cout << "WARNING: Достигнут лимит итераций (" << max_iterations << ")"
              << std::endl;
  }
}
