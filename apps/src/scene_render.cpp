#include "scene_render.hpp"
#include <array>
#include <climits>
#include <vector>

#include "raylib.h"
#include "rcamera.h"
struct Size {
  int width{-1};
  int height{-1};
  int depth{-1};
};

struct Box {
  std::array<int, 6> scores = {
      INT_MAX, INT_MAX, INT_MAX, INT_MAX,
      INT_MAX, INT_MAX}; // оценка коробки, типо там по высоте, по  насколько
                         // она хорошо помещатеся и т.д. - это для типо функции
                         // по подсчету оценки да
  Vector3 pos;
  Size size;
  // поворота, [1][x] с 90 по x, [2][x] с 90 по y,
  int temp_xz[2]; // временные координаты для оценки коробки в зоне
  int mass = 0;   // масса коробки
  int rotate; // если 1 то кабы она повернута да. Указывает на массив xyz_size
              // какой из двух использовать
  bool full_rotateble = false; // полный поворот
  bool placed = false;         // РАЗМЩЕНА ЛИ ЭТА КОРОБКА В ПАЛЛЕТЕ -
  Color color;
};

struct Zone {
  int xyz[3];
  int xyz_size[3]; // ширина[0], высота[1], глубота[2]
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

  std::vector<Box>
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

  int center_mass_or_max_volume = 0; // 0 - центр масс, 1 - объем

  bool hMaxQtyCheck = false; // Если true, то кол-во коробок не ограничено.
  bool lim_lay =
      false; // Запретить неполные слои. Только если hMaxQtyCheck = true.

  // Конструктор для инициализации размеров
  Pallet(int x = 1200, int y = 1555, int z = 800, int maxMass = 1500,
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

void render_box(Pallet &pal) {
  for (auto bx : pal.placed_boxes) {
    const Vector3 center = {bx.pos.x + bx.size.width * 0.5f,
                            bx.pos.y + bx.size.height * 0.5f,
                            bx.pos.z + bx.size.depth * 0.5f};
    DrawCube(center, static_cast<float>(bx.size.width),
             static_cast<float>(bx.size.height),
             static_cast<float>(bx.size.depth), bx.color);
  }
}

void fill_pallet(Pallet &pal) {
  Box bx1;
  bx1.size = {300, 500, 200};
  bx1.pos = {0, 0, 0};
  Color color1 = GRAY;
  bx1.color = color1;

  pal.placed_boxes.push_back(bx1);

  Box bx2;
  bx2.size = {100, 100, 100};
  bx2.pos = {400, 0, 0};
  Color color2 = GREEN;
  bx2.color = color2;
  pal.placed_boxes.push_back(bx2);
}

void test_starter() {
  Pallet pal;
  pal.size = {1200, 1555, 800};
  fill_pallet(pal);
  render_box(pal);
}
