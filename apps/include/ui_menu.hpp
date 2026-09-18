#pragma once
#include <string>
#include <vector>
struct Pallet;
struct Box;

bool DrawMenu(Pallet &pal, std::vector<Box *> &boxes, bool &data_changed);
