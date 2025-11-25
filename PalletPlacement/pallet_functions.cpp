//#include "data.h"
#include "functions.h"
#include <iostream>
#include <array>
#include <algorithm>
#include <cmath>
#include <utility>      // std::move

using namespace std;


void height_map_init(pallet* pal_ptr) {
	int W = pal_ptr->xyz_size[0];
	int D = pal_ptr->xyz_size[2];
	pal_ptr->height_map.assign(W, vector<int>(D, 0)); // инициализируем карту высот нулями
}




bool is_placement_possible(pallet* pallet_ptr) {
	bool flag = false;
	for (int i = 0; i < total_boxes.size(); i++) { // проходимся по коробкам
		if (total_boxes[i]->placed == false) { // если есть хоть одна коробка которая не размещена
			flag = true;
		}
	}
	if (flag == false) {
		return false; // все коробки уже размещены
		cout << "Все коробки уже размещены\n";
	}

	if (pallet_ptr->zone_vector.size() > 0) { // если есть хоть одна живая зона
		//cout << "Есть доступные зоны для размещения коробок\n";
		return true;
	}

	return false;
}



//void split_zone() {
//	return;
//}

void celebrate() {
	cout << "ПЕРЕМОГААА ЛЮДОНЬКІІІ ПЕРЕМОГАА\n";
}




box* select_best_box() {
	box* best_box_ptr = nullptr;
	array<int, SCORES_NUM> best = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX }; // тут храним лучшую коробку
	for (int i = 0; i < total_boxes.size(); i++) { // проходимся по коробкам 
		if (total_boxes.at(i)->scores < best) {
			best = total_boxes.at(i)->scores;
			best_box_ptr = total_boxes.at(i);
		}
	}
	if (best_box_ptr == nullptr || best == array<int, SCORES_NUM>{INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX}) { // если не нашли ни одной коробки
		cout << "Не удалось найти подходящую коробку для размещенияEEE\n";
		return nullptr;
	}

	cout << "Лучшая коробка получила оценки: ";
	cout << best[0] << " " << best[1] << " " << best[2] << " " << best[3] << endl;
	return best_box_ptr;
}

void place_box(pallet* pal_ptr, zone* zone_ptr, box* box_ptr) { // размещаем коробку в зону
	box_ptr->xyz[0] = box_ptr->temp_xz[0];
	box_ptr->xyz[1] = zone_ptr->xyz[1];
	box_ptr->xyz[2] = box_ptr->temp_xz[1];

	pal_ptr->placed_boxes.push_back(box_ptr); // добавляем коробку в паллет

	
	if (pal_ptr->max_height_box == INT_MAX) {
		pal_ptr->max_height_box = pal_ptr->placed_boxes[0]->xyz_size[pal_ptr->placed_boxes[0]->rotate][1];
	}
	pal_ptr->max_height_box = std::max(pal_ptr->max_height_box, box_ptr->xyz[1] + box_ptr->xyz_size[box_ptr->rotate][1]);
	// обновляем макс высоту коробки на паллете
	total_boxes.erase(
		remove(total_boxes.begin(), total_boxes.end(), box_ptr),
		total_boxes.end()
	); // удаляем коробку из списка доступных коробок
	box_ptr->placed = true; // отмечаем что коробка размещена

	cout << "Максимальная высота коробок на паллете теперь: " << pal_ptr->max_height_box << endl;
	
	pal_ptr->placed_since_meb++; // увеличиваем счетчик успешных укладок с последней дефрагации
	pal_ptr->was_defrag = false;

	for (int x = box_ptr->xyz[0]; x < box_ptr->xyz[0] + box_ptr->xyz_size[box_ptr->rotate][0]; x++) {
		for (int z = box_ptr->xyz[2]; z < box_ptr->xyz[2] + box_ptr->xyz_size[box_ptr->rotate][2]; z++) {
			pal_ptr->height_map[x][z] = std::max(pal_ptr->height_map[x][z], box_ptr->xyz[1] + box_ptr->xyz_size[box_ptr->rotate][1]);
		}
	} // обновляем карту высот паллета

	center_mass_calculate(pal_ptr, box_ptr); // обновляем центр масс паллета

}

CenterMassResult simulate_center_mass(const pallet* pal_ptr, int box_mass, double cx_box, double cy_box, double cz_box) {
	CenterMassResult res{};

	int prev_mass = pal_ptr->total_mass;
	int new_mass = prev_mass + box_mass;
	res.total_mass = new_mass;

	double new_cx, new_cy, new_cz;

	if (prev_mass == 0) {
		// Если на паллете ничего не было – центр масс = центр этой коробки
		new_cx = cx_box;
		new_cy = cy_box;
		new_cz = cz_box;
	}
	else {
		new_cx = (pal_ptr->xyz_mass_centre[0] * (double)prev_mass + cx_box * box_mass)
			/ (double)new_mass;
		new_cy = (pal_ptr->xyz_mass_centre[1] * (double)prev_mass + cy_box * box_mass)
			/ (double)new_mass;
		new_cz = (pal_ptr->xyz_mass_centre[2] * (double)prev_mass + cz_box * box_mass)
			/ (double)new_mass;
	}

	res.cx = new_cx;
	res.cy = new_cy;
	res.cz = new_cz;

	return res;
}

void center_mass_calculate(pallet* pal_ptr, box* box_ptr) {
	int r = box_ptr->rotate;

	double bx = box_ptr->xyz[0];
	double by = box_ptr->xyz[1];
	double bz = box_ptr->xyz[2];

	double bw = box_ptr->xyz_size[r][0];
	double bh = box_ptr->xyz_size[r][1];
	double bd = box_ptr->xyz_size[r][2];

	// геометрический центр коробки
	double cx_box = bx + bw / 2.0;
	double cy_box = by + bh / 2.0;
	double cz_box = bz + bd / 2.0;

	// используем общую функцию "симуляции"
	CenterMassResult cm = simulate_center_mass(pal_ptr, box_ptr->mass, cx_box, cy_box, cz_box);

	// уже "коммитим" изменения в паллету
	pal_ptr->total_mass = cm.total_mass;
	pal_ptr->xyz_mass_centre[0] = (int)std::round(cm.cx);
	pal_ptr->xyz_mass_centre[1] = (int)std::round(cm.cy);
	pal_ptr->xyz_mass_centre[2] = (int)std::round(cm.cz);

	cout << "Новый центр масс паллета: (" << pal_ptr->xyz_mass_centre[0] << ", "
		<< pal_ptr->xyz_mass_centre[1] << ", "
		<< pal_ptr->xyz_mass_centre[2] << ") с массой " << pal_ptr->total_mass << endl;
}

box* box_placement_handle(pallet* pal_ptr, zone* zone_ptr) { 
	box* cur_box_ptr;
	int y = zone_ptr->xyz[1];
		for (int i = 0; i < total_boxes.size(); i++) { // проходимся по коробкам 
			cout << "Проверяем коробку размером (" << total_boxes.at(i)->xyz_size[0][0] << ", " << total_boxes.at(i)->xyz_size[0][1] << ", " << total_boxes.at(i)->xyz_size[0][2] << ")\n";
			if (total_boxes.at(i)->placed == true) { //какого то хуя эта коробка уже находится на паллете
				cout << "коробка уже размещена" << endl;
				continue; // ладно похуй, скип
			}
			if (total_boxes.at(i)->mass + pal_ptr->total_mass > pal_ptr->max_mass) { // если коробка слишком тяжелая для паллета
				cout << total_boxes.at(i)->mass << " + " << pal_ptr->total_mass << " > " << pal_ptr->max_mass << endl;
				cout << "коробка слишком тяжелая для паллета\n";
				continue; // скипаем эту коробку
			}

			array<int, SCORES_NUM> best = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX };
			bool any_fit = false;
			int best_rot = 0;
			int x, z;
			for (int j = 0; j < 2; j++) { // проходимся по поворотам коробки
				int w = total_boxes.at(i)->xyz_size[j][0], h = total_boxes.at(i)->xyz_size[j][1],
					d = total_boxes.at(i)->xyz_size[j][2];

				if (!can_place_box_in_zone(zone_ptr, w, h, d) == true) { // если коробка вообще влазит в зону  && (h + pal_ptr->max_height_box) < pal_ptr->xyz_size[1]
					continue;
				}

				
				int out_x, out_z; // сюда вернём координаты верхнего-левого угла коробки
				if (!can_place_box_height_map(pal_ptr, zone_ptr, w, d, out_x, out_z)) { // если коробка не может быть поддержана снизу
					cout << "Коробка не может висеть в воздухе\n";
					continue;
				}

				if (!fits_without_collision(out_x, zone_ptr->xyz[1], out_z, w, h, d, pal_ptr->placed_boxes))continue; // если коробка сталкивается с уже размещенными коробками

				auto res = assess_box_in_zone(zone_ptr, out_x, zone_ptr->xyz[1], out_z, w, h, d, total_boxes.at(i)->mass, pal_ptr);



				if (res < best) {
					best = res;
					best_rot = j;
					x = out_x;
					z = out_z;
				}
				any_fit = true;
			}
			if (any_fit) {
				total_boxes[i]->rotate = best_rot;
				total_boxes[i]->scores = best;
				total_boxes[i]->temp_xz[0] = x;
				total_boxes[i]->temp_xz[1] = z;

				cout << "Выбран для box[" << i << "]: "
					<< best[0] << " " << best[1] << " "
					<< best[2] << " " << best[3] << " (rot=" << best_rot << ")\n";
			}
			else {
				total_boxes[i]->scores = { INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX };
			}
		}

	cur_box_ptr = select_best_box();

	cout << "Выбрана коробка " << cur_box_ptr->xyz_size[0] << " " << cur_box_ptr->xyz_size[1] << " " << cur_box_ptr->xyz_size[2] << endl;

	if (cur_box_ptr == nullptr) {
		pal_ptr->failed_in_row++; // увеличиваем счетчик неудачных попыток, после дефрагации
		return nullptr; // не удалось найти коробку
	}


	place_box(pal_ptr, zone_ptr, cur_box_ptr);
	// ну дальше размещаем коробку да 

	return cur_box_ptr;
}
int get_max_remaining_box_height() {
	int max_h = 0;
	for (auto* b : total_boxes) {
		if (b->placed) continue;
		// обе ориентации, на всякий случай
		max_h = std::max(max_h, b->xyz_size[0][1]);
		max_h = std::max(max_h, b->xyz_size[1][1]);
	}
	return max_h;
}
array<int, SCORES_NUM> assess_box_in_zone(zone* zone_ptr, int bx, int by, int bz, int bw, int bh, int bd, int mass, pallet* pal_ptr) {

	double cx_box = bx + bw / 2.0;
	double cy_box = by + bh / 2.0;
	double cz_box = bz + bd / 2.0;
	
	CenterMassResult cm = simulate_center_mass(pal_ptr, mass, cx_box, cy_box, cz_box);
	// идеальный центр масс паллета


	// отклонение от центра по XZ
	double dx = cm.cx - pal_ptr->ideal_cx;
	double dz = cm.cz - pal_ptr->ideal_cz;
	double dist2_xz = dx * dx + dz * dz;

	// превращаем в "очки": меньше = лучше
	int com_y_score = (int)std::round(cm.cy * 100.0);      // ниже центр масс по Y
	int com_center_score = (int)std::round(dist2_xz * 100.0);   // ближе к центру по XZ

	int w_zone = zone_ptr->xyz_size[0] - (bx - zone_ptr->xyz[0]);
	int y_zone = zone_ptr->xyz_size[1] - (by - zone_ptr->xyz[1]);
	int d_zone = zone_ptr->xyz_size[2] - (bz - zone_ptr->xyz[2]);

	int height_diff = zone_ptr->xyz[1] + bh;
	int waste = w_zone * d_zone - bw * bd;
	int long_side = (std::max)(w_zone - bw, d_zone - bd);
	int short_side = (std::min)(w_zone - bw, d_zone - bd);

	int max_remaining_box_height = get_max_remaining_box_height();
	if (max_remaining_box_height > bh && (pal_ptr->xyz_size[1] - (by + bh)) < max_remaining_box_height) {
		// если после этой укладки не останется места для самой высокой оставшейся коробки
		height_diff += 10000; // штрафуем сильно
	}

	return array<int, SCORES_NUM>{com_y_score + height_diff, com_center_score, height_diff, waste, long_side, short_side };
}

bool can_place_box_height_map(
	pallet* pal_ptr,
	zone* zone,
	int     bw,        // ширина коробки по X
	int     bd,        // глубина коробки по Z
	int& out_x,     // сюда вернём X верхнего-левого угла
	int& out_z      // сюда вернём Z верхнего-левого угла
)
{
	int x0 = zone->xyz[0];
	int y0 = zone->xyz[1];
	int z0 = zone->xyz[2];

	int wz = zone->xyz_size[0]; // размер зоны по X
	int dz = zone->xyz_size[2]; // размер зоны по Z

	// Если зона на дне – любая позиция ок, можно просто вернуть "угол зоны"
	if (y0 == 0) {
		out_x = x0;
		out_z = z0;
		return true;
	}

	int palW = pal_ptr->xyz_size[0];
	int palD = pal_ptr->xyz_size[2];

	int max_start_x = x0 + wz - bw; // Зоны в которых можно разместить коробку
	int max_start_z = z0 + dz - bd;

	if (max_start_x < x0 || max_start_z < z0) {
		return false;
	}

	for (int px = x0; px <= max_start_x; ++px) {
		for (int pz = z0; pz <= max_start_z; ++pz) {

			int supported_area = 0;
			int total_area = bw * bd;
			bool out_of_bounds = false;

			for (int x = px; x < px + bw; ++x) {
				for (int z = pz; z < pz + bd; ++z) {
					// Проверка выхода за паллету
					if (x < 0 || x >= palW || z < 0 || z >= palD) {
						out_of_bounds = true;
						break;
					}

					if (pal_ptr->height_map[x][z] == y0) {
						++supported_area;
					}
				}
				if (out_of_bounds) break;
			}

			if (out_of_bounds) {
				continue;
			}

			double ratio = static_cast<double>(supported_area) / static_cast<double>(total_area);

			if (ratio >= MIN_SUPPORT) {
				// Нашли первую подходящую позицию — возвращаем её
				out_x = px;
				out_z = pz;
				return true;
			}
		}
	}

	return false;
}

bool can_place_box_in_zone(zone* zone, int w, int h, int d) {
	cout << "Проверяю можно ли поместить коробку размером (" << w << ", " << h << ", " << d << ") в зону размером (" << zone->xyz_size[0] << ", " << zone->xyz_size[1] << ", " << zone->xyz_size[2] << ")\n";
	cout << "Координаты зоны " << "(" << zone->xyz[0] << ", " << zone->xyz[1] << ", " << zone->xyz[2] << ")\n";
	if (w <= zone->xyz_size[0] &&
		h <= zone->xyz_size[1] &&
		d <= zone->xyz_size[2]) {
		cout << "Проверил, что коробка помещается\n";
		return true;
	}
	cout << "Проверил, что коробка НЕ помещается\n";
	
	return false;

}


zone* select_zone(pallet* pallet_ptr) {
	for (int i = 0; i < pallet_ptr->zone_vector.size(); i++) {
		if (pallet_ptr->zone_vector.at(i)->usable == true) {
			//cout << "Выбрана зона для размещения коробок\n";
			return pallet_ptr->zone_vector.at(i);
		}
	}
	return nullptr;
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

