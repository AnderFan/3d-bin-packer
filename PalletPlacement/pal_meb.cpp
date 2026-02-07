#include "functions.h"
#include <iostream>
#include <array>
#include <algorithm>
#include <cmath>
#include <utility>  

struct AABB { int x, y, z, w, d, h; };

void meb_gen(pallet* pal_ptr) {
	replace_zones_with_meb(pal_ptr);   // ← MEB-регенерация
	pal_ptr->placed_since_meb = 0;
	pal_ptr->failed_in_row = 0;
	pal_ptr->was_defrag = true;
	cout << "Пошла дефрагация";
	for (auto& v : pal_ptr->zone_vector) {
		cout << "Зона имеет размеры ";
		cout << "(" << v->xyz_size[0] << ", " << v->xyz_size[1] << ", " << v->xyz_size[2] << ") ";
		cout << "(" << v->xyz[0] << ", " << v->xyz[1] << ", " << v->xyz[2] << ")" << endl;
	}
}

void check_meb(pallet* pal_ptr, vector<box*> total_boxes) {
	int SUCCESS_PLACE = total_boxes.size() < 20 ? 10 : total_boxes.size() / 2;
	int FAILER_PLACE = total_boxes.size() < 20 ? 5 : total_boxes.size() / 3;;

	if ((pal_ptr->placed_since_meb >= SUCCESS_PLACE || pal_ptr->failed_in_row >= FAILER_PLACE) && pal_ptr->was_defrag == false) {
		meb_gen(pal_ptr);
	}

}



inline bool aabb_intersect(const AABB& a, const AABB& b) {
	return !(a.x + a.w <= b.x || b.x + b.w <= a.x ||
		a.y + a.d <= b.y || b.y + b.d <= a.y ||
		a.z + a.h <= b.z || b.z + b.h <= a.z);
}

inline bool aabb_empty(const AABB& a) {
	return a.w <= 0 || a.d <= 0 || a.h <= 0;
}

// true = ВНУТРИ этой ячейки нет ни одной уложенной коробки
bool empty_of_boxes(const AABB& cell, const std::vector<box*>& placed) {
	for (auto* b : placed) {
		const int bx = b->xyz[0], by = b->xyz[1], bz = b->xyz[2];
		const auto& s = b->xyz_size[b->rotate]; // учли реальную ориентацию
		AABB bb{ bx, by, bz, s[0], s[1], s[2] };
		if (aabb_intersect(cell, bb)) return false; // любая коллизия → не пусто
	}
	return true;
}

static void uniq_sort(std::vector<int>& v) {
	std::sort(v.begin(), v.end());
	v.erase(std::unique(v.begin(), v.end()), v.end());
}

AABB grow_meb_from_node(int xi, int yi, int zi,
	const std::vector<int>& Xs,
	const std::vector<int>& Ys,
	const std::vector<int>& Zs,
	const std::vector<box*>& placed)
{
	// Границы индексов
	if (xi + 1 >= (int)Xs.size() || yi + 1 >= (int)Ys.size() || zi + 1 >= (int)Zs.size()) {
		return AABB{ 0,0,0,0,0,0 };
	}

	// 1) минимальная ячейка
	AABB cur{ Xs[xi], Ys[yi], Zs[zi],
			  Xs[xi + 1] - Xs[xi],
			  Ys[yi + 1] - Ys[yi],
			  Zs[zi + 1] - Zs[zi] };

	if (aabb_empty(cur) || !empty_of_boxes(cur, placed)) {
		return AABB{ 0,0,0,0,0,0 };
	}

	// 2) Растяжка по X: пробуем растянуть вправо до каждого следующего координата
	for (int xj = xi + 2; xj < (int)Xs.size(); ++xj) {  // ← ПРАВИЛЬНО: xj идёт от xi+2
		AABB test = cur;
		test.w = Xs[xj] - cur.x;  // растянули до Xs[xj]
		if (!empty_of_boxes(test, placed)) break;  // упёрлись в коробку
		cur = test;  // приняли растяжку
	}

	// 3) растяжка по Y
	for (int yj = yi + 2; yj < (int)Ys.size(); ++yj) {
		AABB test = cur;
		test.d = Ys[yj] - cur.y;
		if (!empty_of_boxes(test, placed)) break;
		cur = test;
	}

	// 4) растяжка по Z
	for (int zj = zi + 2; zj < (int)Zs.size(); ++zj) {
		AABB test = cur;
		test.h = Zs[zj] - cur.z;
		if (!empty_of_boxes(test, placed)) break;
		cur = test;
	}

	return cur;
}


// Строим новый список зон (zone*) из MEB-боксов на основании УЖЕ уложенных коробок.
// Эти зоны заменят текущий zone_vector → «дефрагментация».
std::vector<zone*> build_meb_zones(pallet* pal) {
	// 1) Сбор опорных координат: границы контейнера + грани всех установленных коробок
	std::vector<int> Xs{ 0, pal->xyz_size[0] }; // Например, если паллета по X имеет длину 800 мм, и у тебя стоят коробки: первая от x=0 до x=400, вторая от x=400 до x=800, то XS  { 0, 400, 800}
	std::vector<int> Ys{ 0, pal->xyz_size[1] };
	std::vector<int> Zs{ 0, pal->xyz_size[2] };

	for (auto* b : pal->placed_boxes) {
		const auto& s = b->xyz_size[b->rotate];
		Xs.push_back(b->xyz[0]);         Xs.push_back(b->xyz[0] + s[0]);
		Ys.push_back(b->xyz[1]);         Ys.push_back(b->xyz[1] + s[1]);
		Zs.push_back(b->xyz[2]);         Zs.push_back(b->xyz[2] + s[2]);
	}
	uniq_sort(Xs); uniq_sort(Ys); uniq_sort(Zs);

	// 2) Растяжка из каждого узла сетки
	std::vector<zone*> out;
	out.reserve((Xs.size() - 1) * (Ys.size() - 1) * (Zs.size() - 1) / 8); // грубая оценка

	for (int xi = 0; xi < (int)Xs.size() - 1; ++xi)
		for (int yi = 0; yi < (int)Ys.size() - 1; ++yi)
			for (int zi = 0; zi < (int)Zs.size() - 1; ++zi) {
				AABB meb = grow_meb_from_node(xi, yi, zi, Xs, Ys, Zs, pal->placed_boxes);
				if (!aabb_empty(meb)) {
					// Преобразуем AABB в zone
					out.push_back(new zone{
						{meb.x, meb.y, meb.z},      // xyz
						{meb.w, meb.d, meb.h},      // xyz_size
						true                         // usable
						});
				}
			}
	return out;
}
bool fits_without_collision(int bx, int by, int bz, int w, int h, int d,
	const vector<box*>& placed)
{
	AABB cand{ bx, by, bz, w, h, d };
	for (auto* b : placed) {
		const auto& s = b->xyz_size[b->rotate];
		AABB bb{ b->xyz[0], b->xyz[1], b->xyz[2], s[0], s[1], s[2] };
		if (aabb_intersect(cand, bb)) return false;
	}
	return true;
}
