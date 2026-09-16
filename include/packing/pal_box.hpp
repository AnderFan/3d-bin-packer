#pragma once
#include "data.hpp"

void height_map_init(Pallet *pal_ptr);

void center_mass_calculate(Pallet *pal_ptr, Box *box_ptr);

void height_map_init(Pallet *pal_ptr);

bool is_placement_possible(Pallet *pallet_ptr, std::vector<Box *> &total_boxes);

Box *box_placement_handle(Pallet *pal_ptr, Zone *zone_ptr,
                          std::vector<Box *> &total_boxes);
