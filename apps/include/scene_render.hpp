#pragma once
#include "types.hpp"
struct Color;
struct Box;
struct Vector3;
void DrawPalletGrid(Size size, float step);

::Color GetBoxColor(int id);

::Vector3 GetPosCenter(Box *box);
