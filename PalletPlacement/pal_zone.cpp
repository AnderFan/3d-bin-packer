#include "functions.h"
#include <iostream>
#include <array>
#include <algorithm>
#include <cmath>
#include <utility>      // std::move


zone* select_zone(pallet* pallet_ptr) { 
	zone* best = nullptr;
	int best_score = INT_MAX;
	for (auto* z : pallet_ptr->zone_vector) { // Проходка по всем зона
		if (!z->usable) continue; 
		int vol = z->xyz_size[0] * z->xyz_size[1] * z->xyz_size[2]; // высчитываем объём
		int score = vol + z->xyz[1] * 100; // подсчитываем счёт, если объём одинаковый, то мы будем выбирать ту зону, что ниже
		if (score < best_score) {
			best_score = score;
			best = z;
		}
	}
	return best;
}

void kill_zone(pallet* pallet_ptr, zone* zone) {
	pallet_ptr->zone_vector.erase(
		remove(pallet_ptr->zone_vector.begin(), pallet_ptr->zone_vector.end(), zone),
		pallet_ptr->zone_vector.end()
	);
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
	return false;
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