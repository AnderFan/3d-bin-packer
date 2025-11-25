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
	cout << "КОРОБКА УЛОЖЕНА!\n";
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

	CenterMassResult cm = simulate_center_mass(pal_ptr, box_ptr->mass, cx_box, cy_box, cz_box);

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
			double ratio;
			for (int j = 0; j < 2; j++) { // проходимся по поворотам коробки
				int w = total_boxes.at(i)->xyz_size[j][0], h = total_boxes.at(i)->xyz_size[j][1],
					d = total_boxes.at(i)->xyz_size[j][2];

				if (!can_place_box_in_zone(zone_ptr, w, h, d) == true) { // если коробка вообще влазит в зону  && (h + pal_ptr->max_height_box) < pal_ptr->xyz_size[1]
					continue;
				}

				
				int out_x, out_z; // сюда вернём координаты верхнего-левого угла коробки
				if (!can_place_box_height_map(pal_ptr, zone_ptr, w, d, out_x, out_z, ratio)) { // если коробка не может быть поддержана снизу
					cout << "Коробка не может висеть в воздухе\n";
					continue;
				}

				if (!fits_without_collision(out_x, zone_ptr->xyz[1], out_z, w, h, d, pal_ptr->placed_boxes))continue; // если коробка сталкивается с уже размещенными коробками

				auto res = assess_box_in_zone(zone_ptr, out_x, zone_ptr->xyz[1], out_z, w, h, d, total_boxes.at(i)->mass, ratio, pal_ptr);



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
array<int, SCORES_NUM> assess_box_in_zone(zone* zone_ptr, int bx, int by, int bz, int bw, int bh, int bd, int mass, double ratio, pallet* pal_ptr) {

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
	if (ratio < 0.8) height_diff += 500; // если ratfio меньше 0.8, штрафуем
	return array<int, SCORES_NUM>{com_y_score + height_diff, com_center_score, height_diff, waste, long_side, short_side };
}

bool can_place_box_height_map(
	pallet* pal_ptr,
	zone* zone,
	int     bw,        // ширина коробки по X
	int     bd,        // глубина коробки по Z
	int& out_x,     // сюда вернём X верхнего-левого угла
	int& out_z,      // сюда вернём Z верхнего-левого угла
	double& ratio
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

			double t_ratio = static_cast<double>(supported_area) / static_cast<double>(total_area);

			if (t_ratio >= MIN_SUPPORT) {
				// Нашли первую подходящую позицию — возвращаем её
				out_x = px;
				out_z = pz;
				ratio = t_ratio;
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
