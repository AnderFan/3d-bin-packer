#include "data.hpp"
#include "pal_handle.hpp"
#include "viewer_app.hpp"
#include <vector>

struct box_property {
  int Quantity; // кол-во коробок

  int width;
  int height; // высота, глубина, ширина
  int depth;

  int weight; // вес коробки

  bool full_rotateble = false; // полный поворот
};

std::vector<Box *> fun(std::vector<box_property> box_types) {
  std::vector<Box *> boxes;
  for (const auto &box_type : box_types) {
    int Quantity = box_type.Quantity;

    for (int i = 0; i < Quantity; ++i) {
      Box *new_box = new Box();
      new_box->size.width = box_type.width;
      new_box->size.height = box_type.height;
      new_box->size.depth = box_type.depth;

      new_box->mass = box_type.weight;
      new_box->placed = false;
      new_box->full_rotateble = box_type.full_rotateble;

      boxes.push_back(new_box);
    }
  }
  return boxes;
}

int main(void) {
  Pallet pal;

  // std::vector<Box *> total_box = fun({{10, 300, 200, 100, 4},
  //                                     {20, 100, 100, 100, 4},
  //                                     {30, 200, 300, 100, 5},
  //                                     {20, 150, 200, 150},
  //                                     {10, 400, 300, 400}});
  //
  // Setting setting = {};
  // pallet_handle(&pal, total_box);
  ViewerApp(1280, 720, pal);
}
