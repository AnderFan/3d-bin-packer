#pragma once
#include "data.hpp"
#include <vector>
struct Pallet;
struct Box;

bool DrawMenu(Pallet &pal, std::vector<Box *> &boxes, Setting &Setting);
