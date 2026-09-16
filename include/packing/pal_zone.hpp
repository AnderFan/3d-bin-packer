#pragma once
#include "data.hpp"

Zone *select_zone(Pallet *pallet_ptr);

void kill_zone(Pallet *pallet_ptr, Zone *z);

void zone_cleanup(Pallet *pal_ptr); // чистим зоны от мусора.

void split_zone(Pallet *pallet_ptr, Zone *zone_to_split_pointer, Box *box_ptr);

bool merge_any_pair_XYZ(Pallet *pal);

void replace_zones_with_meb(Pallet *pal);

bool is_zone_sane(
    const Pallet *pal,
    const Zone *z); // проверяем зону на адекватность, чтобы не было
