#pragma once

#include "types.hpp"
#include <array>
#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#define SCORES_NUM                                                             \
  6 // сколько у нас всего параметров оценки у коробки - РАЗМЕР МАССИВА С ОЧКАМИ
#define SCORE_HEIGHT_IND 0     // на каком индексе у нас очки по высоте
#define SCORE_WASTE_AREA_IND 1 // на каком индексе очки по вытесняемой площади
#define SCORE_LONG_SIDE_IND 2
#define SCORE_SHORT_SIDE_IND 3

#define PALLET_X 1200
#define PALLET_Y 1555
#define PALLET_Z 800
#define PALLET_MAX_MASS 1050 // Максимальный вес паллеты по умолчанию (кг)

struct Setting {
  int max_iterations = 1000;
  int max_failed = 100;
};

struct CenterMassResult {
  double cx;
  double cy;
  double cz;
  int total_mass;
};
struct Box {
  std::array<int, 6> scores = {
      INT_MAX, INT_MAX, INT_MAX, INT_MAX,
      INT_MAX, INT_MAX}; // оценка коробки, типо там по высоте, по  насколько
                         // она хорошо помещатеся и т.д. - это для типо функции
                         // по подсчету оценки да
  Packing::Vector3 pos;
  Size size;
  Packing::Vector3 temp_pos; // временные координаты для оценки коробки в зоне
  double ratio;
  int mass = 0; // масса коробки
  int rotate;   // если 1 то кабы она повернута да. Указывает на массив xyz_size
                // какой из двух использовать
  bool full_rotateble = false; // полный поворот
  bool placed = false;         // РАЗМЩЕНА ЛИ ЭТА КОРОБКА В ПАЛЛЕТЕ -
};

struct Zone {
  Packing::Vector3 pos;
  Size size;
  // int index = 0;
  bool usable = true; // жива ли зона - если фалс то зона метрва да
};

struct Pallet {
  // int xyz_size[3] = { 1200, 1555, 800 }; // ширина[0], высота[1], глубота[2]
  Size size;
  double xyz_mass_centre[3] = {0, 0, 0}; // центр массы паллета

  double ideal_cx; // идеальный центр
  double ideal_cy;
  double ideal_cz;

  int total_mass = 0;  // суммарный вес
  int max_mass = 1000; // макс вес паллета (теперь изменяемый)

  std::vector<Box *>
      placed_boxes; // размещенные коробоки
                    // vector<zone*> zone_vector = { new zone{ {0, 0, 0},
                    // {xyz_size[0], xyz_size[1], xyz_size[2]}, true} }; //
                    // вектор с активными зонами
  std::vector<Zone *> zone_vector;
  // unique_ptr<zone> zone_vector = make_unique<zone>(zone{ {0,0,0},{10,10,10},
  // true });
  std::vector<Zone *> zone_dead_vector; // мертвые зоны - зоны на которых
                                        // невозможно размеситть коробок

  int max_height_box =
      INT_MAX; // высота самой высокой коробки размещенной на паллете

  int placed_since_meb = 0; // сколько коробок уложено с последней дефрагации
  int failed_in_row = 0;    // сколько подряд неудач найти место

  bool was_defrag = false; // была ли дефрагментация
  // std::map<pair<int, int>, pair<int, int>> mru_positions;
  std::vector<std::vector<int>>
      height_map; // Вектор который хранит y в точке  [x][z]
  float min_support = 0.8f;
  int center_mass_or_max_volume = 0; // 0 - центр масс, 1 - объем

  bool hMaxQtyCheck = false; // Если true, то кол-во коробок не ограничено.
  bool lim_lay =
      false; // Запретить неполные слои. Только если hMaxQtyCheck = true.

  // Конструктор для инициализации размеров
  Pallet(int x = 1200, int y = 1555, int z = 800, int maxMass = 150000,
         int centerMassOrMaxVolume = 0, bool hMaxQtyCheck = false,
         bool lim_lay = false)
      : size{x, y, z}, max_mass(maxMass),
        center_mass_or_max_volume(centerMassOrMaxVolume),
        hMaxQtyCheck(hMaxQtyCheck), lim_lay(lim_lay) {
    ideal_cx = x / 2.0;
    ideal_cy = 0.0;
    ideal_cz = z / 2.0;
    zone_vector.push_back(new Zone{{0, 0, 0}, {x, y, z}, true});
  }
};

// extern vector<box*> total_boxes; // Все коробки которые возомжно разместить
