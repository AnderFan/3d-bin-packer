#pragma once

#include "types.hpp"
#include <array>
#include <climits>
#include <vector>
#define SCORES_NUM 6 // сколько у нас всего параметров оценки у коробки

struct Setting {
  bool hMaxQtyCheck = false; // Если true, то кол-во коробок не ограничено.
  bool lim_lay =
      false; // Запретить неполные слои. Только если hMaxQtyCheck = true.
  int center_mass_or_max_volume = 0; // 0 - центр масс, 1 - объем
  float min_support = 0.8f;
  int max_iterations = 1000;
  int max_failed = 100;
};

struct CenterMassResult {
  Packing::Vector3 pos;
  int total_mass;
};
struct Box {
  std::array<int, 6> scores = {INT_MAX, INT_MAX, INT_MAX,
                               INT_MAX, INT_MAX, INT_MAX};
  Packing::Vector3 pos;
  Size size;
  double ratio = 1.0;
  int mass = 0;
  bool full_rotateble = false;
  bool placed = false;
};

struct Zone {
  Packing::Vector3 pos;
  Size size;
  bool usable = true;
};

struct Pallet {
  Size size;
  Packing::Vector3 mass_centre = {0, 0, 0};

  Packing::Vector3 ideal_pos;

  int total_mass = 0;
  int max_mass = 1000;

  int placed_since_meb = 0;
  int failed_in_row = 0;

  bool was_defrag = false;
  // была ли дефрагментация

  std::vector<Box *> placed_boxes;
  std::vector<Zone *> zone_vector;
  std::vector<Zone *> zone_dead_vector;
  int max_height_box = 0;
  std::vector<std::vector<int>> height_map;

  Pallet(int x = 1200, int y = 1555, int z = 800, int maxMass = 150000)
      : size{x, y, z}, max_mass(maxMass) {
    ideal_pos = {x / 2, 0, z / 2};
    zone_vector.push_back(new Zone{{0, 0, 0}, {x, y, z}, true});
  }
};

struct PlacementCandidate {
  Box *box = nullptr;
  Packing::Vector3 pos{};
  Size size{};
  double ratio = 1.0;
  std::array<int, SCORES_NUM> score = [] {
    std::array<int, SCORES_NUM> a;
    a.fill(INT_MAX);
    return a;
  }();
};
