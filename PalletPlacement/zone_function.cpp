#include "functions.h"
#include <iostream>
#include <array>
#include <algorithm>
#include <cmath>

zone* select_zone(pallet* pallet_ptr) {
	for (int i = 0; i < pallet_ptr->zone_vector.size(); i++) {
		if (pallet_ptr->zone_vector.at(i)->usable == true) {
			//cout << "Выбрана зона для размещения коробок\n";
			return pallet_ptr->zone_vector.at(i);
		}
	}
	return nullptr;
}

void check_meb(pallet* pal_ptr) {
	int SUCCESS_PLACE = total_boxes.size() / 2;
	int FAILER_PLACE = total_boxes.size() / 2;

	if ((pal_ptr->placed_since_meb >= SUCCESS_PLACE || pal_ptr->failed_in_row >= FAILER_PLACE) && pal_ptr->was_defrag == false) {
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

}

void kill_zone(pallet* pallet_ptr, zone* zone) {
	pallet_ptr->zone_vector.erase(
		remove(pallet_ptr->zone_vector.begin(), pallet_ptr->zone_vector.end(), zone),
		pallet_ptr->zone_vector.end()
	);
}

void split_zone(pallet* pallet_ptr, zone* zone_to_split_pointer, box* box_ptr) {
	zone_to_split_pointer->usable = false;
	pallet_ptr->zone_dead_vector.push_back(zone_to_split_pointer); // добавляем зону в мертвые зоны
	kill_zone(pallet_ptr, zone_to_split_pointer); // удаляем зону из активных зон

	auto r = box_ptr->rotate;
	const int x = box_ptr->xyz[0], y = box_ptr->xyz[1], z = box_ptr->xyz[2];
	const int sx = box_ptr->xyz_size[r][0], sy = box_ptr->xyz_size[r][1], sz = box_ptr->xyz_size[r][2];
	//const int zx = zone_to_split_pointer->xyz[0], zy = zone_to_split_pointer->xyz[1], zz = zone_to_split_pointer->xyz[2];
	const int zsx = zone_to_split_pointer->xyz_size[0], zsy = zone_to_split_pointer->xyz_size[1], zsz = zone_to_split_pointer->xyz_size[2];
	const int zx = zone_to_split_pointer->xyz[0], zy = zone_to_split_pointer->xyz[1], zz = zone_to_split_pointer->xyz[2];
	const int x2 = x + sx, y2 = y + sy, z2 = z + sz;
	const int zx2 = zx + zsx, zy2 = zy + zsy, zz2 = zz + zsz;

	auto add_zone = [&](int ax, int ay, int az, int asx, int asy, int asz) {
		if (asx <= 0 || asy <= 0 || asz <= 0) return; // отсекаем пустые зоны
		pallet_ptr->zone_vector.push_back(new zone{ {ax, ay, az}, {asx, asy, asz}, true });
		};

	//add_zone(x + sx, y, z, zsx - sx, sy, zsz); // справа
	//add_zone(x, y, z + sz, sx, sy, zsz - sz); // спереди
	//add_zone(x, y + sy, z, sx, zsy - sy, sz); // сверху

	// справа (по X)
	add_zone(x2, y, z, zx2 - x2, zsy, zsz);
	// спереди (по Y)
	add_zone(x, y2, z, x2 - x, zy2 - y2, zsz);
	// сверху (по Z)
	add_zone(x, y, z2, x2 - x, y2 - y, zz2 - z2);
	// Как работает?? Я не ебу! Читай ебучие чертежи


	//zone_cleanup(pallet_ptr);


}

void replace_zones_with_meb(pallet* pal) {
	// 1) построили новый набор зон
	auto newZones = build_meb_zones(pal);

	// 2) удалили старые зоны (raw* → важно освободить память)
	for (auto* z : pal->zone_vector) delete z;
	pal->zone_vector.clear();

	// 3) заменили на новые
	pal->zone_vector = std::move(newZones);

	// 4) лёгкая санитарка (твои уже готовые функции)
	clipping(pal);                                  // обрезать по габаритам паллеты
	sort_by_xyz_then_size(pal->zone_vector);        // стабильность порядка
	remove_contained(pal->zone_vector);             // убрать зоны, полностью внутри других
	erase_dub(pal);                                 // убрать точные дубли (с delete хвоста)
	merge_zone(pal->zone_vector);                   // (опционально) слить касающиеся блоки
	clipping(pal);                                  // финальный safety-pass
}

void sort_by_xyz_then_size(std::vector<zone*>& zones) { // сортируем зоны по координатам и размерам
	std::sort(zones.begin(), zones.end(), [](zone* a, zone* b) {
		if (a->xyz[1] != b->xyz[1]) return a->xyz[1] < b->xyz[1]; // Сначала Y
		if (a->xyz[0] != b->xyz[0]) return a->xyz[0] < b->xyz[0]; // потом X
		if (a->xyz[2] != b->xyz[2]) return a->xyz[2] < b->xyz[2]; // потом Z
		if (a->xyz_size[0] != b->xyz_size[0]) return a->xyz_size[0] < b->xyz_size[0]; // потом ширина
		if (a->xyz_size[1] != b->xyz_size[1]) return a->xyz_size[1] < b->xyz_size[1]; // потом глубина
		return a->xyz_size[2] < b->xyz_size[2]; // потом высота
		});
}
bool zones_equal(const zone* a, const zone* b) { // проверяем равенство зон
	return a->xyz[0] == b->xyz[0] &&
		a->xyz[1] == b->xyz[1] &&
		a->xyz[2] == b->xyz[2] &&
		a->xyz_size[0] == b->xyz_size[0] &&
		a->xyz_size[1] == b->xyz_size[1] &&
		a->xyz_size[2] == b->xyz_size[2];
}

bool intersect(const zone* a, const zone* b, zone& I) {
	int x1 = std::max(a->xyz[0], b->xyz[0]); // Пересечение начинается там, где начинается «позже» из двух зон;							 
	int y1 = std::max(a->xyz[1], b->xyz[1]); // то есть — правее из двух левых граней.
	int z1 = std::max(a->xyz[2], b->xyz[2]);

	int x2 = std::min(a->xyz[0] + a->xyz_size[0], b->xyz[0] + b->xyz_size[0]); // правая грань пересечения 
	int y2 = std::min(a->xyz[1] + a->xyz_size[1], b->xyz[1] + b->xyz_size[1]); // это самая левая из правых граней двух зон
	int z2 = std::min(a->xyz[2] + a->xyz_size[2], b->xyz[2] + b->xyz_size[2]);

	if (x2 <= x1 || y2 <= y1 || z2 <= z1)
		return false; // нет пересечения

	I.xyz[0] = x1; I.xyz[1] = y1; I.xyz[2] = z1;
	I.xyz_size[0] = x2 - x1;
	I.xyz_size[1] = y2 - y1;
	I.xyz_size[2] = z2 - z1;
	return true;  // Довольно сложная логика если не знать математику, помогал джпт
}

vector<zone*> subtract(const zone* A, const zone& I) { // Создаёт куски зоны A без пересечения с I.
	vector<zone*> out;

	int ax = A->xyz[0], ay = A->xyz[1], az = A->xyz[2];
	int aw = A->xyz_size[0], ad = A->xyz_size[1], ah = A->xyz_size[2];
	int ax2 = ax + aw, ay2 = ay + ad, az2 = az + ah;

	int ix = I.xyz[0], iy = I.xyz[1], iz = I.xyz[2];
	int iw = I.xyz_size[0], id = I.xyz_size[1], ih = I.xyz_size[2];
	int ix2 = ix + iw, iy2 = iy + id, iz2 = iz + ih;

	auto add = [&](int x, int y, int z, int w, int d, int h) {
		if (w <= 0 || d <= 0 || h <= 0) return;
		out.push_back(new zone{ {x,y,z},{w,d,h},true });
		};

	// left
	if (ix > ax) add(ax, ay, az, ix - ax, ad, ah);
	// right
	if (ix2 < ax2) add(ix2, ay, az, ax2 - ix2, ad, ah);
	// back
	if (iy > ay) add(std::max(ax, ix), ay, az, std::min(ax2, ix2) - std::max(ax, ix), iy - ay, ah);
	// front
	if (iy2 < ay2) add(std::max(ax, ix), iy2, az, std::min(ax2, ix2) - std::max(ax, ix), ay2 - iy2, ah);
	// bottom
	if (iz > az) add(std::max(ax, ix), std::max(ay, iy), az,
		std::min(ax2, ix2) - std::max(ax, ix),
		std::min(ay2, iy2) - std::max(ay, iy),
		iz - az);
	// top
	if (iz2 < az2) add(std::max(ax, ix), std::max(ay, iy), iz2,
		std::min(ax2, ix2) - std::max(ax, ix),
		std::min(ay2, iy2) - std::max(ay, iy),
		az2 - iz2);

	return out;
}
void sub_zone(pallet* pal_ptr) {
	// Разрезаем пересекающиеся зоны
	bool changed = true;
	while (changed) {
		changed = false;
		for (size_t i = 0; i < pal_ptr->zone_vector.size() && !changed; ++i) {
			for (size_t j = i + 1; j < pal_ptr->zone_vector.size(); j++) {
				if (pal_ptr->zone_vector[i] == pal_ptr->zone_vector[j]) continue;
				zone I{};
				if (intersect(pal_ptr->zone_vector[i], pal_ptr->zone_vector[j], I)) {
					auto parts = subtract(pal_ptr->zone_vector[i], I);
					delete pal_ptr->zone_vector[i];
					pal_ptr->zone_vector.erase(pal_ptr->zone_vector.begin() + i);
					pal_ptr->zone_vector.insert(pal_ptr->zone_vector.begin() + i, parts.begin(), parts.end());
					changed = true;
					break;
				}
			}
		}
	}
}

bool contained(const zone* a, const zone* b) {
	return b->xyz[0] <= a->xyz[0] &&
		b->xyz[1] <= a->xyz[1] &&
		b->xyz[2] <= a->xyz[2] &&
		a->xyz[0] + a->xyz_size[0] <= b->xyz[0] + b->xyz_size[0] &&
		a->xyz[1] + a->xyz_size[1] <= b->xyz[1] + b->xyz_size[1] &&
		a->xyz[2] + a->xyz_size[2] <= b->xyz[2] + b->xyz_size[2];
}

void remove_contained(std::vector<zone*>& zs) { // Удаляет из zs зоны, полностью содержащиеся в других зонах.
	for (size_t i = 0; i < zs.size(); ++i) {
		for (size_t j = 0; j < zs.size(); ++j) {
			if (i == j) continue;
			if (contained(zs[i], zs[j])) {
				delete zs[i];
				zs.erase(zs.begin() + i);
				--i;
				break;
			}
		}
	}
}

void clipping(pallet* pal_ptr) {
	auto clip = [&](zone* zone) {
		int& x = zone->xyz[0], y = zone->xyz[1], z = zone->xyz[2];
		int& sx = zone->xyz_size[0], sy = zone->xyz_size[1], sz = zone->xyz_size[2];

		int X2 = std::min(x + sx, pal_ptr->xyz_size[0]);
		int Y2 = std::min(y + sy, pal_ptr->xyz_size[1]);
		int Z2 = std::min(z + sz, pal_ptr->xyz_size[2]); // Узнаю что меньше, величина паллеты или зоны

		x = std::max(x, 0);
		y = std::max(y, 0);
		z = std::max(z, 0); // Подтягиваем координаты к нулю, если надо

		sz = Z2 - z;
		sx = X2 - x;
		sy = Y2 - y; // Пересчитываем размеры зоны

		};
	for (auto* zone : pal_ptr->zone_vector) clip(zone);

	pal_ptr->zone_vector.erase(
		remove_if(pal_ptr->zone_vector.begin(), pal_ptr->zone_vector.end(),
			[](zone* zone) {
				return zone->xyz_size[0] <= 0 || zone->xyz_size[1] <= 0 || zone->xyz_size[2] <= 0;
			}),
		pal_ptr->zone_vector.end()
	);

}

bool can_mergeX(const zone* A, const zone* B) {
	return A->xyz[1] == B->xyz[1] && A->xyz[2] == B->xyz[2] &&
		A->xyz_size[1] == B->xyz_size[1] &&
		A->xyz_size[2] == B->xyz_size[2] &&
		(A->xyz[0] + A->xyz_size[0] == B->xyz[0] ||
			B->xyz[0] + B->xyz_size[0] == A->xyz[0]);
}

zone* mergeX(const zone* A, const zone* B) {
	int x = std::min(A->xyz[0], B->xyz[0]);
	int y = A->xyz[1];
	int z = A->xyz[2];
	int w = A->xyz_size[0] + B->xyz_size[0];
	int d = A->xyz_size[1];
	int h = A->xyz_size[2];
	return new zone{ {x,y,z},{w,d,h},true };
}
inline bool can_mergeY(const zone* A, const zone* B) {
	// Совпадают по X и Z-полосе и размерам по X,Z
	bool same_band =
		A->xyz[0] == B->xyz[0] &&            // x
		A->xyz[2] == B->xyz[2] &&            // z
		A->xyz_size[0] == B->xyz_size[0] &&  // w
		A->xyz_size[2] == B->xyz_size[2];    // h

	if (!same_band) return false;

	// Касаются по целой грани вдоль Y (без зазора и без перекрытия):
	// ...A...|B  или  B|...A...
	bool touch =
		(A->xyz[1] + A->xyz_size[1] == B->xyz[1]) ||
		(B->xyz[1] + B->xyz_size[1] == A->xyz[1]);

	return touch;
}

inline zone* mergeY(const zone* A, const zone* B) {
	// Предполагается, что can_mergeY(A,B) == true
	int x = A->xyz[0];
	int z = A->xyz[2];
	int y = std::min(A->xyz[1], B->xyz[1]);             // нижняя грань по Y
	int w = A->xyz_size[0];                              // одинаковые у A и B
	int h = A->xyz_size[2];                              // одинаковые у A и B
	int d = A->xyz_size[1] + B->xyz_size[1];            // суммируем глубину (по Y)

	return new zone{ {x, y, z}, {w, d, h}, true };
}



inline bool can_mergeZ(const zone* A, const zone* B) {
	// Совпадают по X и Y-полосе и размерам по X,Y
	bool same_band =
		A->xyz[0] == B->xyz[0] &&            // x
		A->xyz[1] == B->xyz[1] &&            // y
		A->xyz_size[0] == B->xyz_size[0] &&  // w
		A->xyz_size[1] == B->xyz_size[1];    // d

	if (!same_band) return false;

	bool touch =
		(A->xyz[2] + A->xyz_size[2] == B->xyz[2]) ||
		(B->xyz[2] + B->xyz_size[2] == A->xyz[2]);

	return touch;
}

inline zone* mergeZ(const zone* A, const zone* B) {
	// Предполагается, что can_mergeZ(A,B) == true
	int x = A->xyz[0];
	int y = A->xyz[1];
	int z = std::min(A->xyz[2], B->xyz[2]);             // нижняя грань по Z
	int w = A->xyz_size[0];                              // одинаковые у A и B
	int d = A->xyz_size[1];                              // одинаковые у A и B
	int h = A->xyz_size[2] + B->xyz_size[2];            // суммируем высоту (по Z)

	return new zone{ {x, y, z}, {w, d, h}, true };
}

bool try_merge_once(vector<zone*>& zs) {
	for (size_t i = 0; i < zs.size(); ++i) {
		for (size_t j = i + 1; j < zs.size(); ++j) {
			zone* A = zs[i], * B = zs[j];
			zone* M = nullptr;

			if (can_mergeX(A, B)) M = mergeX(A, B);
			else if (can_mergeY(A, B)) M = mergeY(A, B);
			else if (can_mergeZ(A, B)) M = mergeZ(A, B);

			if (M) {
				delete A; delete B;
				zs.erase(zs.begin() + j);
				zs.erase(zs.begin() + i);
				zs.insert(zs.begin() + i, M);
				return true; // сделали одно слияние — выходим, начнём заново
			}
		}
	}
	return false;
}

void merge_zone(vector<zone*>& pal_ptr) {
	sort_by_xyz_then_size(pal_ptr);

	bool changed = true;
	while (changed) changed = try_merge_once(pal_ptr);
}

void erase_dub(pallet* pal_ptr) {
	auto& v = pal_ptr->zone_vector;
	sort_by_xyz_then_size(v);
	auto it = std::unique(v.begin(), v.end(), zones_equal);
	for (auto d = it; d != v.end(); ++d) delete* d;  // освободить память
	v.erase(it, v.end()); // делет 

}

void zone_cleanup(pallet* pal_ptr) { // чистим зоны от мусора. 
	// P.S от подсмешка: эта фукнция далась мне крайне трудно и болезненно. Некоторые  вещи из тех что написаны
	// здесь я до сих пор не до конца понимаю

	clipping(pal_ptr); // обрезаем зоны по размерам паллеты
	sort_by_xyz_then_size(pal_ptr->zone_vector); // сортируем зоны по координатам и размерам
	erase_dub(pal_ptr); // удаляем дублирующиеся зоны
	sub_zone(pal_ptr); // Вырезаем кусочки пересекающихся зон UPD: Возможно стоит просто убирать самую большую зону.



	sort_by_xyz_then_size(pal_ptr->zone_vector);
	remove_contained(pal_ptr->zone_vector); // удаляем зоны которые полностью содержатся в других зонах


	merge_zone(pal_ptr->zone_vector); // Мердж соседних зон

	//// После мерджа опять чистим от мусора
	clipping(pal_ptr);
	remove_contained(pal_ptr->zone_vector);
	sort_by_xyz_then_size(pal_ptr->zone_vector); // сортируем зоны по координатам и размерам
	erase_dub(pal_ptr); // удаляем дублирующиеся зоны
	clipping(pal_ptr);

	//clipping(pal_ptr); // обрезаем зоны по размерам паллеты
	//sub_zone(pal_ptr); // Вырезаем кусочки пересекающихся зон
	//remove_contained(pal_ptr->zone_vector); // удаляем зоны которые полностью содержатся в других зонах
	//merge_zone(pal_ptr->zone_vector); // Мердж соседних зон
	//clipping(pal_ptr); // обрезаем зоны по размерам паллеты
	//remove_contained(pal_ptr->zone_vector);
	//sub_zone(pal_ptr); // Вырезаем кусочки пересекающихся зон
	//clipping(pal_ptr);
}

bool merge_any_pair_XYZ(pallet* pal) { // попытка чё-нибудь слить, что бы не расстраиваться
	auto& zs = pal->zone_vector;
	sort_by_xyz_then_size(zs);

	for (size_t i = 0; i < zs.size(); ++i) {
		for (size_t j = i + 1; j < zs.size(); ++j) {
			zone* A = zs[i];
			zone* B = zs[j];
			zone* M = nullptr;

			if (can_mergeX(A, B)) M = mergeX(A, B);
			else if (can_mergeY(A, B)) M = mergeY(A, B);
			else if (can_mergeZ(A, B)) M = mergeZ(A, B);

			if (M) {
				delete A; delete B;
				zs.erase(zs.begin() + j);
				zs.erase(zs.begin() + i);
				zs.insert(zs.begin() + i, M);

				sort_by_xyz_then_size(zs);
				remove_contained(zs);
				cout << "Слили зону для увеличения шансов размещения коробок\n";
				return true;
			}
		}
	}
	//cout << "Не удалось слить ни одну пару зон\n";
	return false;
}


// ДАЛЬШЕ MEB-регенерация. Уже не так страшно как то что выше, МОЖНО ВЫДЫХАТЬ!!

struct AABB { int x, y, z, w, d, h; };

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

// Сетка задаётся тремя отсортированными массивами координат Xs, Ys, Zs.
// Узел сетки – это тройка индексов (xi, yi, zi) -> точка (Xs[xi], Ys[yi], Zs[zi]).
//
// grow_meb_from_node строит МАКСИМАЛЬНЫЙ пустой параллелепипед,
// который начинается в узле (xi, yi, zi):
// 1) стартуем минимальной ячейкой [Xs[xi], Xs[xi+1]) × [Ys[yi], Ys[yi+1]) × [Zs[zi], Zs[zi+1])
// 2) растягиваем её вправо по X, пока пусто
// 3) затем растягиваем по Y, пока пусто
// 4) затем по Z, пока пусто
AABB grow_meb_from_node(int xi, int yi, int zi,
	const std::vector<int>& Xs,
	const std::vector<int>& Ys,
	const std::vector<int>& Zs,
	const std::vector<box*>& placed)
{
	// Границы индексов: следующий узел справа/вперёд/вверх должен существовать
	if (xi + 1 >= (int)Xs.size() || yi + 1 >= (int)Ys.size() || zi + 1 >= (int)Zs.size()) {
		return AABB{ 0,0,0,0,0,0 }; // нет элементарной ячейки
	}

	// 1) минимальная ячейка от текущего узла до ближайших правых Xs/Ys/Zs
	AABB cur{ Xs[xi], Ys[yi], Zs[zi],
			  Xs[xi + 1] - Xs[xi],
			  Ys[yi + 1] - Ys[yi],
			  Zs[zi + 1] - Zs[zi] };

	// Если ячейка нулевая или в ней уже стоит коробка — с этого узла ничего не вырастет
	if (aabb_empty(cur) || !empty_of_boxes(cur, placed)) return AABB{ 0,0,0,0,0,0 };

	// 2) Растяжка по X: пробуем двигать правую грань на следующий Xs,
	//    каждый раз проверяя пустоту ВСЕГО нового объёма.
	int xj = xi + 1;
	while (xj + 1 < (int)Xs.size()) {
		AABB test = cur;
		test.w = Xs[xj + 1] - cur.x; // растянули вправо
		if (!empty_of_boxes(test, placed)) break; // упёрлись в коробку/границу
		cur = test; // приняли растяжку
		++xj;
	}

	// 3) растяжка по Y (вперёд)
	int yj = yi + 1;
	while (yj + 1 < (int)Ys.size()) {
		AABB test = cur;
		test.d = Ys[yj + 1] - cur.y;
		if (!empty_of_boxes(test, placed)) break;
		cur = test;
		++yj;
	}

	// 4) растяжка по Z (вверх)
	int zj = zi + 1;
	while (zj + 1 < (int)Zs.size()) {
		AABB test = cur;
		test.h = Zs[zj + 1] - cur.z;
		if (!empty_of_boxes(test, placed)) break;
		cur = test;
		++zj;
	}

	// cur — максимальный пустой параллелепипед, "приросший" из узла
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
					// Преобразуем AABB в твою zone
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

