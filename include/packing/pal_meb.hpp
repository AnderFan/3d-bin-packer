#pragma once
#include "data.hpp"
void check_meb(Pallet *pal_ptr, const std::vector<Box *> &total_boxes);

void meb_gen(Pallet *pal_ptr);

std::vector<Zone *> build_meb_zones(Pallet *pal);

bool fits_without_collision(Packing::Vector3 temp_pos, Size box_size,
                            const std::vector<Box *> &placed);
